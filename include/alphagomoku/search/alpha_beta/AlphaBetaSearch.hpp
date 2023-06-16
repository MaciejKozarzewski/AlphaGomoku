/*
 * AlphaBetaSearch.hpp
 *
 *  Created on: Jun 2, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_ALPHABETASEARCH_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_ALPHABETASEARCH_HPP_

#include <alphagomoku/networks/NNUE.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
#include <alphagomoku/search/alpha_beta/SharedHashTable.hpp>
#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <cassert>
#include <algorithm>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	class AlphaBetaSearch
	{
		private:
			ActionStack generator_stack;

			ActionStack action_stack;

			int max_positions = 0; // maximum number of positions that will be searched
			int position_counter = 0;
			MoveGeneratorMode movegen_mode;

			GameConfig game_config;
			PatternCalculator pattern_calculator;
			MoveGenerator move_generator;
			nnue::InferenceNNUE inference_nnue;

			SharedHashTable shared_table;
			HashKey128 hash_key;

			size_t total_positions = 0;
			size_t total_calls = 0;
			std::vector<std::pair<Move, int>> move_updates;
			TimedStat total_time;
		public:
			AlphaBetaSearch(const GameConfig &gameConfig, MoveGeneratorMode mode);
			void increaseGeneration();
			void loadWeights(const nnue::NNUEWeights &weights);
			int solve(SearchTask &task, int maxDepth, int maxPositions);
			void print_stats() const;
		private:
			Score recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions);
			bool is_move_legal(Move m) const noexcept;
			Score evaluate();
			void update_patterns();
			void make_move(Move m);
			void undo_move(Move m);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_ALPHABETASEARCH_HPP_ */
