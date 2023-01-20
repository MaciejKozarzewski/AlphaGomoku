/*
 * SupervisedLearning.cpp
 *
 *  Created on: Mar 12, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Board.hpp>

#include <minml/core/Tensor.hpp>
#include <minml/core/Shape.hpp>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace
{
	using namespace ag;

	void augment_data(matrix<Sign> &board, matrix<float> &policyTarget, matrix<Value> &actionValues)
	{
		int r = randInt(4 + 4 * static_cast<int>(board.isSquare()));
		augment(board, r);
		augment(policyTarget, r);
		augment(actionValues, r);
	}
}

namespace ag
{
	SupervisedLearning::SupervisedLearning(const TrainingConfig &config) :
			training_loss(3),
			training_accuracy(5),
			validation_loss(3),
			validation_accuracy(5),
			config(config)
	{
	}
	void SupervisedLearning::updateTrainingStats(float pl, float vl, std::vector<float> &acc)
	{
		training_loss.at(0) += 1.0f;
		training_loss.at(1) += pl;
		training_loss.at(2) += vl;
		addVectors(training_accuracy, acc);
	}
	void SupervisedLearning::updateValidationStats(float pl, float vl, std::vector<float> &acc)
	{
		validation_loss.at(0) += 1.0f;
		validation_loss.at(1) += pl;
		validation_loss.at(2) += vl;
		addVectors(validation_accuracy, acc);
	}
	void SupervisedLearning::train(AGNetwork &model, GameBuffer &buffer, int steps)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		ml::Shape shape = model.getInputShape();
		int batch_size = shape[0];
		int rows = shape[1];
		int cols = shape[2];

		std::vector<int> training_sample_order = permutation(buffer.size());
		size_t training_sample_index = 0;

		matrix<Sign> board(rows, cols);
		matrix<Sign> board_copy(rows, cols);

		matrix<float> policy_target;
		matrix<Value> action_values_target;
		Value value_target;
		for (int i = 0; i < steps; i++)
		{
			if ((i + 1) % (steps / 10) == 0)
				std::cout << i + 1 << '\n';
			for (int b = 0; b < batch_size; b++)
			{
				const SearchData &sample = buffer.getFromBuffer(training_sample_order.at(training_sample_index)).getSample();
				training_sample_index++;
				if (training_sample_index >= training_sample_order.size())
				{
					training_sample_order = permutation(training_sample_order.size());
					training_sample_index = 0;
				}

				sample.getBoard(board);
				policy_target = prepare_policy_target(sample);
				action_values_target = prepare_action_values_target(sample);
				value_target = prepare_value_target(sample);

				if (config.augment_training_data)
					augment_data(board, policy_target, action_values_target);

//				sample.print();
//				std::cout << "Training target\n";
//				std::cout << Board::toString(board) << '\n';
//				std::cout << Board::toString(board, policy_target) << '\n';
//				std::cout << Board::toString(board, action_values_target) << '\n';
//				std::cout << "Value target = " << value_target.toString() << '\n';

				model.packInputData(b, board, sample.getMove().sign);
				model.packTargetData(b, policy_target, action_values_target, value_target);
			}
			model.forward(batch_size);

			auto accuracy = model.getAccuracy(batch_size, 4);
			model.backward(batch_size);
			std::vector<float> loss = model.getLoss(batch_size);
			updateTrainingStats(loss.at(0), loss.at(1), accuracy);

			learning_steps++;
		}
	}
	void SupervisedLearning::validate(AGNetwork &model, GameBuffer &buffer)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		ml::Shape shape = model.getInputShape();
		int batch_size = shape[0];
		int rows = shape[1];
		int cols = shape[2];

		matrix<Sign> board(rows, cols);

		int counter = 0;
		while (counter < buffer.size())
		{
			int this_batch = 0;
			while (counter < buffer.size() and this_batch < batch_size)
			{
				const SearchData &sample = buffer.getFromBuffer(counter).getSample();

				sample.getBoard(board);
				matrix<float> policy_target = prepare_policy_target(sample);
				matrix<Value> action_values_target = prepare_action_values_target(sample);
				Value value_target = prepare_value_target(sample);

				if (config.augment_training_data)
					augment_data(board, policy_target, action_values_target);

//				sample.print();
//				std::cout << "Training target\n";
//				std::cout << Board::toString(board) << '\n';
//				std::cout << Board::toString(board, policy_target) << '\n';
//				std::cout << Board::toString(board, action_values_target) << '\n';
//				std::cout << "Value target = " << value_target.toString() << '\n';

				model.packInputData(this_batch, board, sample.getMove().sign);
				model.packTargetData(this_batch, policy_target, action_values_target, value_target);

				counter++;
				this_batch++;
			}
			model.forward(batch_size);

			auto accuracy = model.getAccuracy(batch_size);
			std::vector<float> loss = model.getLoss(batch_size);
			updateValidationStats(loss.at(0), loss.at(1), accuracy);
		}
	}
	void SupervisedLearning::saveTrainingHistory(const std::string &workingDirectory)
	{
		std::string path = workingDirectory + "/training_history.txt";
		std::ofstream history_file;
		history_file.open(path.data(), std::ios::app);
		history_file << std::setprecision(9);
		if (learning_steps == 0)
		{
			history_file << "#step";
			history_file << " train_p_loss train_v_loss";
			history_file << " val_p_loss val_v_loss";
			for (size_t i = 0; i < (training_accuracy.size() - 1); i++)
				history_file << " train_top" << (i + 1) << "_acc";
			for (size_t i = 0; i < (validation_accuracy.size() - 1); i++)
				history_file << " val_top" << (i + 1) << "_acc";
			history_file << '\n';
		}
		else
		{
			std::vector<float> train_loss = averageStats(training_loss);
			std::vector<float> train_acc = averageStats(training_accuracy);
			std::vector<float> valid_loss = averageStats(validation_loss);
			std::vector<float> valid_acc = averageStats(validation_accuracy);

			history_file << std::setprecision(6);
			history_file << learning_steps;
			history_file << " " << train_loss.at(0) << " " << train_loss.at(1);
			history_file << " " << valid_loss.at(0) << " " << valid_loss.at(1);
			for (size_t i = 0; i < train_acc.size(); i++)
				history_file << " " << 100 * train_acc.at(i);
			for (size_t i = 0; i < valid_acc.size(); i++)
				history_file << " " << 100 * valid_acc.at(i);
			history_file << '\n';
			history_file.close();
		}
	}
	void SupervisedLearning::clearStats()
	{
		std::fill(training_loss.begin(), training_loss.end(), 0.0f);
		std::fill(training_accuracy.begin(), training_accuracy.end(), 0.0f);
		std::fill(validation_loss.begin(), validation_loss.end(), 0.0f);
		std::fill(validation_accuracy.begin(), validation_accuracy.end(), 0.0f);
	}

	Json SupervisedLearning::saveProgress() const
	{
		Json result;
		result["learning_steps"] = learning_steps;
		return result;
	}
	void SupervisedLearning::loadProgress(const Json &json)
	{
		learning_steps = json["learning_steps"];
	}
	/*
	 * private
	 */
	matrix<float> SupervisedLearning::prepare_policy_target(const SearchData &sample)
	{
		matrix<float> policy(sample.rows(), sample.cols());
		matrix<ProvenValue> proven_values(sample.rows(), sample.cols());

		sample.getPolicy(policy);
		sample.getActionProvenValues(proven_values);
		for (int i = 0; i < policy.size(); i++)
		{
			switch (proven_values[i])
			{
				case ProvenValue::UNKNOWN:
					break;
				case ProvenValue::LOSS:
					policy[i] = 1.0e-6f;
					break;
				case ProvenValue::DRAW:
					break;
				case ProvenValue::WIN:
					policy[i] = 1e6f;
					break;

			}
		}
		normalize(policy);
		return policy;
	}
	matrix<Value> SupervisedLearning::prepare_action_values_target(const SearchData &sample)
	{
		matrix<Value> action_values(sample.rows(), sample.cols());
		matrix<ProvenValue> proven_values(sample.rows(), sample.cols());

		sample.getActionValues(action_values);
		sample.getActionProvenValues(proven_values);

		for (int i = 0; i < action_values.size(); i++)
		{
			switch (proven_values[i])
			{
				case ProvenValue::UNKNOWN:
					break;
				case ProvenValue::LOSS:
					action_values[i] = Value(0.0f, 0.0f, 1.0f);
					break;
				case ProvenValue::DRAW:
					action_values[i] = Value(0.0f, 1.0f, 0.0f);
					break;
				case ProvenValue::WIN:
					action_values[i] = Value(1.0f, 0.0f, 0.0f);
					break;

			}
		}
		return action_values;
	}
	Value SupervisedLearning::prepare_value_target(const SearchData &sample)
	{
		return convertOutcome(sample.getOutcome(), sample.getMove().sign);
	}
}

