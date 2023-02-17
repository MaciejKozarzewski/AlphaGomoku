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

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace
{
	using namespace ag;

	void augment_data_pack(DataPack &pack)
	{
		const int r = randInt(available_symmetries(pack.board));
		ag::augment(pack.board, r);
		ag::augment(pack.policy, r);
		ag::augment(pack.action_values, r);
	}
	DataPack prepare_data_pack(const SearchData &sample, bool perform_augmentations)
	{
		DataPack result(sample.rows(), sample.cols());
		result.value = convertOutcome(sample.getOutcome(), sample.getMove().sign);
		result.sign_to_move = sample.getMove().sign;

		auto get_expectation = [](Score score, Value value)
		{
			switch (score.getProvenValue())
			{
				case ProvenValue::LOSS:
				return Value::loss().getExpectation();
				case ProvenValue::DRAW:
				return Value::draw().getExpectation();
				default:
				case ProvenValue::UNKNOWN:
				return value.getExpectation();
				case ProvenValue::WIN:
				return Value::win().getExpectation();
			}
		};

		int max_N = 0, sum_N = 0;
		float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;
		for (int row = 0; row < sample.rows(); row++)
			for (int col = 0; col < sample.cols(); col++)
			{
				if (sample.getVisitCount(row, col) > 0 or sample.getActionScore(row, col).isProven())
				{
					const Score score = sample.getActionScore(row, col);
					const Value value = sample.getActionValue(row, col);
					const int visits = std::max(1, sample.getVisitCount(row, col)); // proven action counts as 1 visit (even though it might not have been visited by the search)
					const float policy_prior = sample.getPolicyPrior(row, col);

					sum_v += value.getExpectation() * visits;
					sum_q += policy_prior * get_expectation(score, value);
					sum_p += policy_prior;
					max_N = std::max(max_N, visits);
					sum_N += visits;
				}
			}
		float V_mix = 0.0f;
		if (sum_N > 0) //or sum_p > 0.0f)
		{
			const float inv_N = 1.0f / sum_N;
			V_mix = (sample.getMinimaxValue().getExpectation() - sum_v * inv_N) + (1.0f - inv_N) / sum_p * sum_q;
		}
		else
			V_mix = sample.getMinimaxValue().getExpectation();

		for (int row = 0; row < sample.rows(); row++)
			for (int col = 0; col < sample.cols(); col++)
				result.board.at(row, col) = sample.getSign(row, col);

		float shift = std::numeric_limits<float>::lowest();
		for (int row = 0; row < sample.rows(); row++)
			for (int col = 0; col < sample.cols(); col++)
				if (sample.getSign(row, col) == Sign::NONE)
				{
					const int visits = sample.getVisitCount(row, col);
					const Score score = sample.getActionScore(row, col);
					const Value value = sample.getActionValue(row, col);
					const float policy_prior = sample.getPolicyPrior(row, col);

					float Q;
					switch (score.getProvenValue())
					{
						case ProvenValue::LOSS:
							Q = -100.0f + 0.1f * score.getDistanceToWinOrLoss();
							break;
						case ProvenValue::DRAW:
							Q = Value::draw().getExpectation();
							break;
						default:
						case ProvenValue::UNKNOWN:
							Q = (visits > 0) ? value.getExpectation() : V_mix;
							break;
						case ProvenValue::WIN:
							Q = +101.0f - 0.1f * score.getDistanceToWinOrLoss();
							break;
					}
					const float sigma_Q = (50 + max_N) * Q;
					const float logit = safe_log(policy_prior);

					result.policy.at(row, col) = logit + sigma_Q;
					shift = std::max(shift, result.policy.at(row, col));

					switch (sample.getActionScore(row, col).getProvenValue())
					{
						case ProvenValue::UNKNOWN:
							result.action_values.at(row, col) = sample.getActionValue(row, col);
							break;
						case ProvenValue::LOSS:
							result.action_values.at(row, col) = Value::loss();
							break;
						case ProvenValue::DRAW:
							result.action_values.at(row, col) = Value::draw();
							break;
						case ProvenValue::WIN:
							result.action_values.at(row, col) = Value::win();
							break;
					}
				}
		assert(shift != std::numeric_limits<float>::lowest());

		float inv_sum = 0.0f;
		for (int i = 0; i < result.policy.size(); i++)
			if (result.board[i] == Sign::NONE)
			{
				result.policy[i] = std::exp(std::max(-100.0f, result.policy[i] - shift));
				if (result.policy[i] < 1.0e-9f)
					result.policy[i] = 0.0f;
				inv_sum += result.policy[i];
			}
		assert(inv_sum > 0.0f);
		inv_sum = 1.0f / inv_sum;

		for (int i = 0; i < result.policy.size(); i++)
			result.policy[i] *= inv_sum;

//		for (int row = 0; row < sample.rows(); row++)
//			for (int col = 0; col < sample.cols(); col++)
//			{
//				result.board.at(row, col) = sample.getSign(row, col);
//				switch (sample.getActionScore(row, col).getProvenValue())
//				{
//					case ProvenValue::UNKNOWN:
//						result.policy.at(row, col) = sample.getVisitCount(row, col);
//						result.action_values.at(row, col) = sample.getActionValue(row, col);
//						break;
//					case ProvenValue::LOSS:
//						result.policy.at(row, col) = 1.0e-6f;
//						result.action_values.at(row, col) = Value::loss();
//						break;
//					case ProvenValue::DRAW:
//						result.policy.at(row, col) = sample.getVisitCount(row, col);
//						result.action_values.at(row, col) = Value::draw();
//						break;
//					case ProvenValue::WIN:
//						result.policy.at(row, col) = 1.0e6f;
//						result.action_values.at(row, col) = Value::win();
//						break;
//				}
//			}
//		normalize(result.policy);

//		std::cout << "Sample from buffer\n";
//		sample.print();
//		std::cout << "Prepared training data pack\n";
//		std::cout << "Sign to move " << result.sign_to_move << '\n';
//		std::cout << "Value target = " << result.value.toString() << '\n';
//		std::cout << "Board\n" << Board::toString(result.board);
//		std::cout << "Policy target\n" << Board::toString(result.board, result.policy);
//		std::cout << "Action values target\n" << Board::toString(result.board, result.action_values);
//		exit(0);

		if (perform_augmentations)
			augment_data_pack(result);

		return result;
	}
	void mask_out_target_data_pack(const DataPack &output, DataPack &target)
	{
		assert(equalSize(output.board, target.board));
		assert(equalSize(output.policy, target.policy));
		assert(equalSize(output.action_values, target.action_values));

		for (int i = 0; i < output.action_values.size(); i++)
			if (target.policy[i] == 0.0f) // if all fields are zero it means that we don't have any target data for this spot
				target.action_values[i] = output.action_values[i]; // in such case we set value of the target to the output from the network, zeroing the gradient

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
	void push_input_data_to_model(std::vector<DataPack> &inputs, AGNetwork &model)
	{
		assert(model.getBatchSize() >= static_cast<int>(inputs.size()));
		for (size_t i = 0; i < inputs.size(); i++)
			model.packInputData(i, inputs.at(i).board, inputs.at(i).sign_to_move);
	}
	void push_target_data_to_model(std::vector<DataPack> &targets, AGNetwork &model)
	{
		assert(model.getBatchSize() >= static_cast<int>(targets.size()));
		DataPack output_data_pack(model.getInputShape()[1], model.getInputShape()[2]);
		for (size_t i = 0; i < targets.size(); i++)
		{
			model.unpackOutput(i, output_data_pack.policy, output_data_pack.action_values, output_data_pack.value);
			mask_out_target_data_pack(output_data_pack, targets[i]);
			model.packTargetData(i, targets[i].policy, targets[i].action_values, targets[i].value);
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
	void SupervisedLearning::train(AGNetwork &model, GameBuffer &buffer, int steps)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		const int batch_size = model.getInputShape().firstDim();

		std::vector<int> training_sample_order = permutation(buffer.size());
		size_t training_sample_index = 0;

		std::vector<DataPack> data_packs(batch_size);

		for (int i = 0; i < steps; i++)
		{
			if ((i + 1) % std::max(1, (steps / 10)) == 0)
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

				data_packs.at(b) = prepare_data_pack(sample, config.augment_training_data);
			}

			push_input_data_to_model(data_packs, model);
			model.forward(batch_size);

			push_target_data_to_model(data_packs, model);
			model.backward(batch_size);

			std::vector<float> loss = model.getLoss(batch_size);
			auto accuracy = model.getAccuracy(batch_size, 4);
			updateTrainingStats(loss, accuracy);

			learning_steps++;
		}
	}
	void SupervisedLearning::validate(AGNetwork &model, GameBuffer &buffer)
	{
		ml::Device::cpu().setNumberOfThreads(config.device_config.omp_threads);

		const int batch_size = model.getInputShape().firstDim();

		std::vector<DataPack> data_packs(std::min(batch_size, buffer.size()));

		int counter = 0;
		while (counter < buffer.size())
		{
			int this_batch = 0;
			while (counter < buffer.size() and this_batch < batch_size)
			{
				const SearchData &sample = buffer.getFromBuffer(counter).getSample();
				data_packs.at(this_batch) = prepare_data_pack(sample, config.augment_training_data);
				counter++;
				this_batch++;
			}

			push_input_data_to_model(data_packs, model);
			model.forward(this_batch);

			push_target_data_to_model(data_packs, model);

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

