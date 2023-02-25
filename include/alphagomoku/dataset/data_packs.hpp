/*
 * data_packs.hpp
 *
 *  Created on: Feb 25, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_DATA_PACKS_HPP_
#define ALPHAGOMOKU_DATASET_DATA_PACKS_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class Node;
}

namespace ag
{
	struct SearchDataPack
	{
			matrix<Sign> board;
			matrix<float> policy_prior;
			matrix<int> visit_count;
			matrix<Value> action_values;
			matrix<Score> action_scores;
			Value minimax_value;
			Score minimax_score;
			GameOutcome game_outcome = GameOutcome::UNKNOWN;
			Move played_move;

			SearchDataPack() noexcept = default;
			SearchDataPack(int rows, int cols);
			SearchDataPack(const Node &rootNode, const matrix<Sign> &board);
			void clear();
			void print() const;
	};

	struct TrainingDataPack
	{
			matrix<Sign> board;
			matrix<int> visit_count;
			matrix<float> policy_target;
			matrix<Value> action_values_target;
			Value value_target;
			Sign sign_to_move = Sign::NONE;

			TrainingDataPack() noexcept = default;
			TrainingDataPack(int rows, int cols);
			void clear();
			void print() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_DATA_PACKS_HPP_ */
