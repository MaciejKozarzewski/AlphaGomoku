/*
 * Sampler.cpp
 *
 *  Created on: Feb 25, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>

namespace ag
{
	Sampler::Sampler(const GameDataBuffer &buffer, int batchSize) :
			buffer(buffer),
			search_data_pack(buffer.getConfig().rows, buffer.getConfig().cols),
			game_ordering(permutation(buffer.size())),
			batch_size(batchSize)
	{
	}
	void Sampler::get(TrainingDataPack &result)
	{
		result.clear();
		const int game_index = game_ordering.at(counter);
		const int sample_index = randInt(buffer.getGameData(game_index).numberOfSamples());

		buffer.getGameData(game_index).getSample(search_data_pack, sample_index);
		prepare_training_data(result, search_data_pack);

		counter++;
		if (counter >= game_ordering.size())
		{
			std::random_shuffle(game_ordering.begin(), game_ordering.end());
			counter = 0;
		}
	}
	/*
	 * private
	 */
	void Sampler::prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample)
	{
		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;

		for (int i = 0; i < sample.board.size(); i++)
		{
			result.policy_target[i] = sample.visit_count[i];
			switch (sample.action_scores[i].getProvenValue())
			{
				case ProvenValue::UNKNOWN:
					result.action_values_target[i] = sample.action_values[i];
					break;
				case ProvenValue::LOSS:
					result.action_values_target[i] = Value::loss();
					break;
				case ProvenValue::DRAW:
					result.action_values_target[i] = Value::draw();
					break;
				case ProvenValue::WIN:
					result.action_values_target[i] = Value::win();
					break;
			}
		}
		normalize(result.policy_target);
	}
	void Sampler::prepare_training_data_v2(TrainingDataPack &result, const SearchDataPack &sample)
	{
		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;

		constexpr float scale = 50.0f;

		float max_value = std::numeric_limits<float>::lowest();
		for (int i = 0; i < sample.board.size(); i++)
			if (sample.visit_count[i] > 0)
			{
				result.policy_target[i] = scale * sample.action_values[i].getExpectation();// + safe_log(sample.policy_prior[i]);
				max_value = std::max(max_value, result.policy_target[i]);
			}

		float sum_policy = 0.0f;
		for (int i = 0; i < sample.board.size(); i++)
			if (sample.visit_count[i] > 0)
			{
				result.policy_target[i] = std::exp(result.policy_target[i] - max_value);
				sum_policy += result.policy_target[i];
			}

		for (int i = 0; i < sample.board.size(); i++)
			if (sample.visit_count[i] > 0)
				result.policy_target[i] /= sum_policy;

		for (int i = 0; i < sample.action_scores.size(); i++)
			switch (sample.action_scores[i].getProvenValue())
			{
				case ProvenValue::UNKNOWN:
					result.action_values_target[i] = sample.action_values[i];
					break;
				case ProvenValue::LOSS:
					result.action_values_target[i] = Value::loss();
					break;
				case ProvenValue::DRAW:
					result.action_values_target[i] = Value::draw();
					break;
				case ProvenValue::WIN:
					result.action_values_target[i] = Value::win();
					break;
			}
	}

} /* namespace ag */

