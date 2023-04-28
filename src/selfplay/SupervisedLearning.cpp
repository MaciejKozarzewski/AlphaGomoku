/*
 * SupervisedLearning.cpp
 *
 *  Created on: Mar 12, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace
{
	using namespace ag;

	void augment_data_pack(TrainingDataPack &pack)
	{
		const int r = randInt(available_symmetries(pack.board));
		ag::augment(pack.board, r);
		ag::augment(pack.visit_count, r);
		ag::augment(pack.policy_target, r);
		ag::augment(pack.action_values_target, r);
	}

	void mask_out_target_data_pack(const TrainingDataPack &output, TrainingDataPack &target)
	{
		assert(equalSize(output.visit_count, target.visit_count));
		assert(equalSize(output.action_values_target, target.action_values_target));

		for (int i = 0; i < output.action_values_target.size(); i++)
			if (target.visit_count[i] == 0) // if an action was not visited then we don't have any target data for this spot
				target.action_values_target[i] = output.action_values_target[i]; // in such case we set value of the target to the output from the network, zeroing the gradient

//		std::cout << "Output from network\n";
//		std::cout << "Value output = " << output.value.toString() << '\n';
//		std::cout << "Policy output\n" << Board::toString(matrix<Sign>(15, 15), output.policy);
//		std::cout << "Action values output\n" << Board::toString(matrix<Sign>(15, 15), output.action_values);
//
//		std::cout << "Updated target data pack\n";
//		std::cout << "Sign to move " << target.sign_to_move << '\n';
//		std::cout << "Value target = " << target.value.toString() << '\n';
//		std::cout << "Board\n" << Board::toString(target.board);
//		std::cout << "Policy target\n" << Board::toString(matrix<Sign>(15, 15), target.policy);
//		std::cout << "Action values target\n" << Board::toString(matrix<Sign>(15, 15), target.action_values);
//		std::cout << "\n------------------------------------------------------------------------\n";
	}
	void push_input_data_to_model(int batchSize, std::vector<TrainingDataPack> &inputs, AGNetwork &model)
	{
		assert(model.getBatchSize() >= static_cast<int>(inputs.size()));
		assert(batchSize <= static_cast<int>(inputs.size()));
		for (int i = 0; i < batchSize; i++)
			model.packInputData(i, inputs[i].board, inputs[i].sign_to_move);
	}
	void push_target_data_to_model(int batchSize, std::vector<TrainingDataPack> &targets, AGNetwork &model)
	{
		assert(model.getBatchSize() >= static_cast<int>(targets.size()));
		assert(batchSize <= static_cast<int>(targets.size()));
		TrainingDataPack output_data_pack(model.getInputShape()[1], model.getInputShape()[2]);
		for (int i = 0; i < batchSize; i++)
		{
			model.unpackOutput(i, output_data_pack.policy_target, output_data_pack.action_values_target, output_data_pack.value_target);
			mask_out_target_data_pack(output_data_pack, targets[i]);
			model.packTargetData(i, targets[i].policy_target, targets[i].action_values_target, targets[i].value_target);
		}
	}

}

namespace ag
{
	SupervisedLearning::SupervisedLearning(const TrainingConfig &config) :
			training_loss(4),
			training_accuracy(5),
			validation_loss(4),
			validation_accuracy(5),
			config(config)
	{
	}
	void SupervisedLearning::updateTrainingStats(const std::vector<float> &loss, std::vector<float> &acc)
	{
		training_loss.at(0) += 1.0f;
		for (size_t i = 0; i < loss.size(); i++)
			training_loss.at(1 + i) += loss.at(i);
		addVectors(training_accuracy, acc);
	}
	void SupervisedLearning::updateValidationStats(const std::vector<float> &loss, std::vector<float> &acc)
	{
		validation_loss.at(0) += 1.0f;
		for (size_t i = 0; i < loss.size(); i++)
			validation_loss.at(1 + i) += loss.at(i);
		addVectors(validation_accuracy, acc);
	}
	void SupervisedLearning::train(AGNetwork &model, GameDataBuffer &buffer, int steps)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		const int batch_size = model.getInputShape().firstDim();
		const int rows = model.getInputShape().dim(1);
		const int cols = model.getInputShape().dim(2);

		std::unique_ptr<Sampler> sampler = createSampler(config.sampler_type);
		sampler->init(buffer, batch_size);

		std::vector<TrainingDataPack> data_packs(batch_size, TrainingDataPack(rows, cols));

		for (int i = 0; i < steps; i++)
		{
			if ((i + 1) % std::max(1, (steps / 10)) == 0)
				std::cout << i + 1 << '\n';

			for (int b = 0; b < batch_size; b++)
			{
				sampler->get(data_packs.at(b));
				if (config.augment_training_data)
					augment_data_pack(data_packs.at(b));
			}

			push_input_data_to_model(batch_size, data_packs, model);
			model.forward(batch_size);

			push_target_data_to_model(batch_size, data_packs, model);
			model.backward(batch_size);

			std::vector<float> loss = model.getLoss(batch_size);
			auto accuracy = model.getAccuracy(batch_size, 4);
			updateTrainingStats(loss, accuracy);

			learning_steps++;
			if (hasCapturedSignal(SignalType::INT))
				exit(0);
		}
	}
	void SupervisedLearning::validate(AGNetwork &model, GameDataBuffer &buffer)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		const int batch_size = model.getInputShape().firstDim();
		const int rows = model.getInputShape().dim(1);
		const int cols = model.getInputShape().dim(2);

		std::unique_ptr<Sampler> sampler = createSampler(config.sampler_type);
		sampler->init(buffer, batch_size);

		std::vector<TrainingDataPack> data_packs(batch_size, TrainingDataPack(rows, cols));

		int counter = 0;
		while (counter < buffer.numberOfGames())
		{
			int this_batch = 0;
			while (counter < buffer.numberOfGames() and this_batch < batch_size)
			{
				sampler->get(data_packs.at(this_batch));
				counter++;
				this_batch++;
			}

			push_input_data_to_model(this_batch, data_packs, model);
			model.forward(this_batch);

			push_target_data_to_model(this_batch, data_packs, model);

			std::vector<float> loss = model.getLoss(this_batch);
			auto accuracy = model.getAccuracy(this_batch);
			updateValidationStats(loss, accuracy);
		}
	}
	void SupervisedLearning::saveTrainingHistory(const std::string &workingDirectory)
	{
		if (workingDirectory.empty())
			return;
		std::string path = workingDirectory + "/training_history.txt";
		std::ofstream history_file;
		history_file.open(path.data(), std::ios::app);
		history_file << std::setprecision(9);
		if (learning_steps == 0)
		{
			history_file << "#step";
			history_file << " train_p_loss train_v_loss train_q_loss";
			history_file << " val_p_loss val_v_loss val_q_loss";
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
			for (size_t i = 0; i < train_loss.size(); i++)
				history_file << " " << train_loss.at(i);
			for (size_t i = 0; i < valid_loss.size(); i++)
				history_file << " " << valid_loss.at(i);
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

} /* namespace ag */

