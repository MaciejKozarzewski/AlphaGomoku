/*
 * SupervisedLearning.cpp
 *
 *  Created on: Mar 12, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <libml/Tensor.hpp>
#include <libml/Shape.hpp>
#include <libml/Scalar.hpp>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <thread>
#include <chrono>

namespace ag
{
	SupervisedLearning::SupervisedLearning(const Json &config) :
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
		ml::Shape shape = model.getGraph().getInputShape();
		int batch_size = shape[0];
		int rows = shape[1];
		int cols = shape[2];

		std::vector<int> training_sample_order = permutation(buffer.size());
		size_t training_sample_index = 0;

		std::vector<float> acc(5);
		matrix<Sign> board(rows, cols);
		matrix<float> policy(rows, cols);
		for (int i = 0; i < steps; i++)
		{
			if (i % 100 == 0)
				std::cout << i << '\n';
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
				sample.getPolicy(policy);
				if (config["training_options"]["augment_training_data"])
				{
					int r = randInt(4 + 4 * static_cast<int>(board.isSquare()));
					augment(board, r);
					augment(policy, r);
				}

				model.packData(b, board, policy, sample.getOutcome(), sample.getMove().sign);
			}
			model.forward(batch_size);
			auto accuracy = model.getAccuracy(batch_size, 4);
			model.backward(batch_size);
			std::vector<ml::Scalar> loss = model.getLoss(batch_size);
			updateTrainingStats(loss.at(0).get<float>(), loss.at(1).get<float>(), accuracy);
			learning_steps++;
		}
	}
	void SupervisedLearning::validate(AGNetwork &model, GameBuffer &buffer)
	{
		ml::Shape shape = model.getGraph().getInputShape();
		int batch_size = shape[0];
		int rows = shape[1];
		int cols = shape[2];
		std::vector<float> acc(5);
		matrix<Sign> board(rows, cols);
		matrix<float> policy(rows, cols);
		int counter = 0;
		while (counter < buffer.size())
		{
			int this_batch = 0;
			while (counter < buffer.size() and this_batch < batch_size)
			{
				const SearchData &sample = buffer.getFromBuffer(counter).getSample();

				sample.getBoard(board);
				sample.getPolicy(policy);
				if (config["training_options"]["augment_training_data"])
				{
					int r = randInt(4 + 4 * static_cast<int>(board.isSquare()));
					augment(board, r);
					augment(policy, r);
				}

				model.packData(this_batch, board, policy, sample.getOutcome(), sample.getMove().sign);
				counter++;
				this_batch++;
			}
			model.forward(batch_size);

			auto accuracy = model.getAccuracy(batch_size);
			std::vector<ml::Scalar> loss = model.getLoss(batch_size);
			updateValidationStats(loss.at(0).get<float>(), loss.at(1).get<float>(), accuracy);
		}
	}
	void SupervisedLearning::saveTrainingHistory()
	{
		std::string path = static_cast<std::string>(config["working_directory"]) + "/training_history.txt";
		std::ofstream dataPlik;
		dataPlik.open(path.data(), std::ios::app);
		dataPlik << std::setprecision(9);
		if (learning_steps == 0)
		{
			dataPlik << "#step";
			dataPlik << " train_p_loss train_v_loss";
			dataPlik << " val_p_loss val_v_loss";
			for (size_t i = 0; i < (training_accuracy.size() - 1); i++)
				dataPlik << " train_top" << (i + 1) << "_acc";
			for (size_t i = 0; i < (validation_accuracy.size() - 1); i++)
				dataPlik << " val_top" << (i + 1) << "_acc";
			dataPlik << '\n';
		}
		else
		{
			averageStats(training_loss);
			averageStats(training_accuracy);
			averageStats(validation_loss);
			averageStats(validation_accuracy);

			dataPlik << std::setprecision(6);
			dataPlik << learning_steps;
			dataPlik << " " << training_loss.at(1) << " " << training_loss.at(2);
			dataPlik << " " << validation_loss.at(1) << " " << validation_loss.at(2);
			for (size_t i = 1; i < training_accuracy.size(); i++)
				dataPlik << " " << 100 * training_accuracy.at(i);
			for (size_t i = 1; i < validation_accuracy.size(); i++)
				dataPlik << " " << 100 * validation_accuracy.at(i);
			dataPlik << '\n';
			dataPlik.close();
			std::fill(training_loss.begin(), training_loss.end(), 0.0f);
			std::fill(training_accuracy.begin(), training_accuracy.end(), 0.0f);
			std::fill(validation_loss.begin(), validation_loss.end(), 0.0f);
			std::fill(validation_accuracy.begin(), validation_accuracy.end(), 0.0f);
		}
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
}

