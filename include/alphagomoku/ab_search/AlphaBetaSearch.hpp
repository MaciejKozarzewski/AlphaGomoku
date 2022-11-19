/*
 * AlphaBetaSearch.hpp
 *
 *  Created on: Oct 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_ALPHABETASEARCH_HPP_
#define ALPHAGOMOKU_AB_SEARCH_ALPHABETASEARCH_HPP_

#include <alphagomoku/ab_search/ActionList.hpp>
#include <alphagomoku/ab_search/KillerHeuristic.hpp>
#include <alphagomoku/ab_search/MoveGenerator.hpp>
#include <alphagomoku/ab_search/FastHashFunction.hpp>
#include <alphagomoku/ab_search/LocalHashTable.hpp>
#include <alphagomoku/ab_search/SharedHashTable.hpp>
#include <alphagomoku/ab_search/StaticSolver.hpp>
#include <alphagomoku/ab_search/nnue/NNUE.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <cassert>
#include <algorithm>
#include <iostream>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	struct AlphaBetaSearchStats
	{
			TimedStat setup;
			TimedStat static_solver;
			TimedStat hashtable;
			TimedStat move_generation;
			TimedStat evaluate;
			TimedStat solve;
			int64_t hits = 0;
			int64_t total_positions = 0;
			int64_t local_cache_hits = 0;
			int64_t local_cache_calls = 0;
			int64_t shared_cache_hits = 0;
			int64_t shared_cache_calls = 0;

			AlphaBetaSearchStats();
			std::string toString() const;
	};

	class Measurement
	{
			std::vector<std::pair<int, float>> m_values;
			int m_param_value;
		public:
			Measurement(int paramValue) noexcept;
			void clear() noexcept;
			int getParamValue() const noexcept;
			void update(int x, float y) noexcept;
			std::pair<float, float> predict(int x) const noexcept;
			std::string toString() const;
	};

	struct ABSearchOptions
	{
			bool use_local_table = true;
			bool use_shared_table = true;
			int static_solver_depth = 5;
	};

	class AlphaBetaSearch
	{
		private:
			ActionStack action_stack;

			int number_of_moves_for_draw; // after that number of moves, a draw will be concluded
			int max_positions; // maximum number of positions that will be searched

			double position_counter = 0.0;

			GameConfig game_config;
			PatternCalculator pattern_calculator;
			std::vector<Location> temporary_storage;
			StaticSolver static_solver;
			nnue::InferenceNNUE evaluator;

			LocalHashTable local_table;
			SharedHashTable<4> *shared_table = nullptr;
			HashKey<64> hash_key;

			KillerHeuristics<4> killers;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;
			AlphaBetaSearchStats stats;
		public:
			size_t all_actions = 0;
			MoveGeneratorMode mode = MoveGeneratorMode::NORMAL;
			AlphaBetaSearch(GameConfig gameConfig, int maxPositions = 100);
			~AlphaBetaSearch();

			int64_t getMemory() const noexcept;
			void setSharedTable(SharedHashTable<4> &table) noexcept;
			SharedHashTable<4>& getSharedTable() noexcept;
			/*
			 * \brief
			 *  level = 0 - only game end conditions are checked
			 *  level = 1 - static solver is run on positions
			 *  level = 2 - move generator is used
			 *  level = 3 - search of depth 1 is run
			 *  level > 3 - full search is run
			 */
			std::pair<Move, Score> solve(SearchTask &task, int level = 4);
			void tune(float speed);

			AlphaBetaSearchStats getStats() const;
			void clearStats();
			void print_stats() const
			{
				pattern_calculator.print_stats();
				std::cout << stats.toString() << '\n';
			}
		private:
			Score recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions);
			Score threat_space_search(int depthRemaining, Score alpha, Score beta, ActionList &actions);
			Score evaluate();
			Move get_hash_move(HashKey<64> key) noexcept;
			bool is_move_valid(Move m) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_ALPHABETASEARCH_HPP_ */
