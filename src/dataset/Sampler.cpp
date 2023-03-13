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

namespace
{
	using namespace ag;

	float get_expectation(Score score, Value value) noexcept
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
	}
}

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
		prepare_training_data_old(result, search_data_pack);

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
		const int board_size = sample.board.size();

		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;

		float shift = std::numeric_limits<float>::lowest();
		for (int i = 0; i < board_size; i++)
			if (sample.board[i] == Sign::NONE)
			{
				const int visits = sample.visit_count[i];
				const Value value = sample.action_values[i];
				const float policy_prior = sample.policy_prior[i];

				const float Q = (visits > 0) ? value.getExpectation() : 0.0f;

				result.policy_target[i] = safe_log(policy_prior) + 10.0f * Q;
				shift = std::max(shift, result.policy_target[i]);

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
		assert(shift != std::numeric_limits<float>::lowest());

		float inv_sum = 0.0f;
		for (int i = 0; i < result.policy_target.size(); i++)
			if (result.board[i] == Sign::NONE)
			{
				result.policy_target[i] = std::exp(std::max(-100.0f, result.policy_target[i] - shift));
				if (result.policy_target[i] < 1.0e-9f)
					result.policy_target[i] = 0.0f;
				inv_sum += result.policy_target[i];
			}
		assert(inv_sum > 0.0f);
		inv_sum = 1.0f / inv_sum;

		for (int i = 0; i < result.policy_target.size(); i++)
			result.policy_target[i] *= inv_sum;
	}
	void Sampler::prepare_training_data_old(TrainingDataPack &result, const SearchDataPack &sample)
	{
		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;

		for (int i = 0; i < sample.board.size(); i++)
		{
			switch (sample.action_scores[i].getProvenValue())
			{
				case ProvenValue::UNKNOWN:
					result.policy_target[i] = sample.visit_count[i];
					result.action_values_target[i] = sample.action_values[i];
					break;
				case ProvenValue::LOSS:
					result.policy_target[i] = 1.0e-6f * sample.action_scores[i].getDistance();
					result.action_values_target[i] = Value::loss();
					break;
				case ProvenValue::DRAW:
					result.policy_target[i] = sample.visit_count[i];
					result.action_values_target[i] = Value::draw();
					break;
				case ProvenValue::WIN:
					result.policy_target[i] = 1.0e4f - sample.action_scores[i].getDistance();
					result.action_values_target[i] = Value::win();
					break;
			}
		}
		normalize(result.policy_target);
	}

} /* namespace ag */

