/*
 * Sampler.cpp
 *
 *  Created on: Feb 25, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/dataset/Dataset.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/random.hpp>

#include <iostream>

namespace
{
	using namespace ag;
	Value get_value(Value value, Score score)
	{
		return score.isProven() ? score.convertToValue() : value;
	}
	std::vector<std::pair<int, int>> list_all_games(const Dataset &dataset)
	{
		std::vector<std::pair<int, int>> result;

		const std::vector<int> list_of_buffers = dataset.getListOfBuffers();
		for (size_t i = 0; i < list_of_buffers.size(); i++)
		{
			const GameDataBuffer &buffer = dataset.getBuffer(list_of_buffers[i]);
			for (int j = 0; j < buffer.numberOfGames(); j++)
				result.push_back( { list_of_buffers[i], j });
		}
		return result;
	}
}

namespace ag
{
	void Sampler::init(const Dataset &dataset, int batchSize)
	{
		this->dataset = &dataset;
		buffer_and_game_ordering = list_all_games(dataset);
		std::random_shuffle(buffer_and_game_ordering.begin(), buffer_and_game_ordering.end());
		batch_size = batchSize;
	}
	void Sampler::get(TrainingDataPack &result)
	{
		if (dataset == nullptr)
			throw std::logic_error("Dataset is null. Most likely this Sampler was not initialized");
		result.clear();
		const int buffer_index = buffer_and_game_ordering.at(counter).first;
		const int game_index = buffer_and_game_ordering.at(counter).second;
		const int sample_index = randInt(dataset->getBuffer(buffer_index).getGameData(game_index).numberOfSamples());

		const GameConfig cfg = dataset->getBuffer(buffer_index).getConfig();
		search_data_pack = SearchDataPack(cfg.rows, cfg.cols);
		dataset->getBuffer(buffer_index).getGameData(game_index).getSample(search_data_pack, sample_index);
		prepare_training_data(result, search_data_pack);

//		buffer->getGameData(game_index)[sample_index].print();
//		search_data_pack.print();
//		result.print();

		counter++;
		if (counter >= buffer_and_game_ordering.size())
		{
			std::random_shuffle(buffer_and_game_ordering.begin(), buffer_and_game_ordering.end());
			counter = 0;
		}
	}

	/*
	 * SamplerVisits
	 */
	void SamplerVisits::prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample)
	{
		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;
		assert(sample.moves_left > 0);
		result.moves_left = sample.moves_left;
		result.minimax_target = sample.minimax_value.getExpectation();

		for (int i = 0; i < sample.board.size(); i++)
		{
			const Score score = sample.action_scores[i];
			if (score.isProven())
			{
				result.action_values_target[i] = score.convertToValue();
				result.visit_count[i] = std::max(1, result.visit_count[i]);
			}
			else
				result.action_values_target[i] = sample.action_values[i];

			switch (score.getProvenValue())
			{
				case ProvenValue::LOSS:
					result.policy_target[i] = 1.0e-6f;
					break;
				default:
				case ProvenValue::DRAW:
				case ProvenValue::UNKNOWN:
					result.policy_target[i] = sample.visit_count[i];
					break;
				case ProvenValue::WIN:
					result.policy_target[i] = 1.0e+6f;
					break;
			}
		}
		normalize(result.policy_target);
	}

	/*
	 * SamplerValues
	 */
	void SamplerValues::prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample)
	{
		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.action_values_target.fill(Value::loss());
		result.sign_to_move = sample.played_move.sign;
		assert(sample.moves_left > 0);
		result.moves_left = sample.moves_left;
		result.minimax_target = sample.minimax_value.getExpectation();

		int unvisited_actions_count = 0;
		int max_N = 0.0;
		float sum_PxQ = 0.0f;
		float sum_P = 0.0f;
		for (int i = 0; i < sample.visit_count.size(); i++)
			if (result.board[i] == Sign::NONE)
			{
				if (sample.visit_count[i] > 0 or sample.action_scores[i].isProven())
				{
					const Value q = get_value(sample.action_values[i], sample.action_scores[i]);
					sum_PxQ += sample.policy_prior[i] * q.getExpectation();
					sum_P += sample.policy_prior[i];
					max_N = std::max(max_N, sample.visit_count[i]);
					result.visit_count[i] = std::max(1, result.visit_count[i]); // proven moves can have 0 visits if they were proven by the solver
				}
				else
					unvisited_actions_count++;
			}

		const float minimax_V = get_value(sample.minimax_value, sample.minimax_score).getExpectation();
		sum_PxQ = sum_P * sum_PxQ + (1.0f - sum_P) * minimax_V;

		float max_value = std::numeric_limits<float>::lowest();
		for (int i = 0; i < sample.board.size(); i++)
			if (result.board[i] == Sign::NONE)
			{
				float Q = sum_PxQ;
				float P = std::max(0.0f, (1.0f - sum_P) / unvisited_actions_count);
				if (result.visit_count[i] > 0)
				{
					const Score score = sample.action_scores[i];
					switch (score.getProvenValue())
					{
						case ProvenValue::LOSS:
							Q = -1.0f / (1.0f + score.getDistance());
							break;
						case ProvenValue::DRAW:
							Q = Value::draw().getExpectation();
							break;
						default:
						case ProvenValue::UNKNOWN:
							Q = sample.action_values[i].getExpectation();
							break;
						case ProvenValue::WIN:
							Q = 1.0f + 2.0f / (1.0f + score.getDistance());
							break;
					}
					result.action_values_target[i] = get_value(sample.action_values[i], score);
					P = sample.policy_prior[i];
				}
				result.policy_target[i] = 50.0f * Q + safe_log(P);
				max_value = std::max(max_value, result.policy_target[i]);
			}

		float sum_policy = 0.0f;
		for (int i = 0; i < sample.board.size(); i++)
			if (result.board[i] == Sign::NONE)
			{
				result.policy_target[i] = std::exp(std::max(-40.0f, result.policy_target[i] - max_value));
				sum_policy += result.policy_target[i];
			}

		for (int i = 0; i < sample.board.size(); i++)
			if (result.board[i] == Sign::NONE)
				result.policy_target[i] /= sum_policy;
	}

	std::unique_ptr<Sampler> createSampler(const std::string &name)
	{
		if (name == "visits")
			return std::make_unique<SamplerVisits>();
		if (name == "values")
			return std::make_unique<SamplerValues>();
		throw std::logic_error("Unknown sampler type '" + name + "'");
	}

} /* namespace ag */

