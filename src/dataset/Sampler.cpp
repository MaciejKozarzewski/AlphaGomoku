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

} /* namespace ag */

