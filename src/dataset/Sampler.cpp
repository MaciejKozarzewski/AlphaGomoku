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
		const int board_size = sample.board.size();

		result.board = sample.board;
		result.visit_count = sample.visit_count;
		result.value_target = convertOutcome(sample.game_outcome, sample.played_move.sign);
		result.sign_to_move = sample.played_move.sign;

		int max_N = 0, sum_N = 0;
		float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;
		for (int i = 0; i < board_size; i++)
			if (sample.visit_count[i] > 0 or sample.action_scores[i].isProven())
			{
				const Score score = sample.action_scores[i];
				const Value value = sample.action_values[i];
				const int visits = std::max(1, sample.visit_count[i]); // proven action counts as 1 visit (even though it might not have been visited by the search)
				const float policy_prior = sample.policy_prior[i];

				sum_v += value.getExpectation() * visits;
				sum_q += policy_prior * get_expectation(score, value);
				sum_p += policy_prior;
				max_N = std::max(max_N, visits);
				sum_N += visits;
			}

		float V_mix = 0.0f;
		if (sum_N > 0) //or sum_p > 0.0f)
		{
			const float inv_N = 1.0f / sum_N;
			V_mix = (sample.minimax_value.getExpectation() - sum_v * inv_N) + (1.0f - inv_N) / sum_p * sum_q;
		}
		else
			V_mix = sample.minimax_value.getExpectation();

		float shift = std::numeric_limits<float>::lowest();
		for (int i = 0; i < board_size; i++)
			if (sample.board[i] == Sign::NONE)
			{
				const int visits = sample.visit_count[i];
				const Score score = sample.action_scores[i];
				const Value value = sample.action_values[i];
				const float policy_prior = sample.policy_prior[i];

				float Q;
				switch (score.getProvenValue())
				{
					case ProvenValue::LOSS:
						Q = -100.0f + 0.1f * score.getDistance();
						break;
					case ProvenValue::DRAW:
						Q = Value::draw().getExpectation();
						break;
					default:
					case ProvenValue::UNKNOWN:
						Q = (visits > 0) ? value.getExpectation() : V_mix;
						break;
					case ProvenValue::WIN:
						Q = +101.0f - 0.1f * score.getDistance();
						break;
				}
				const float sigma_Q = (50 + max_N) * Q;
				const float logit = safe_log(policy_prior);

				result.policy_target[i] = logit + sigma_Q;
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
					result.policy_target[i] = 1.0e-6f;
					result.action_values_target[i] = Value::loss();
					break;
				case ProvenValue::DRAW:
					result.policy_target[i] = sample.visit_count[i];
					result.action_values_target[i] = Value::draw();
					break;
				case ProvenValue::WIN:
					result.policy_target[i] = 1.0e6f;
					result.action_values_target[i] = Value::win();
					break;
			}
		}
		normalize(result.policy_target);
	}

} /* namespace ag */

