/*
 * ThreatSpaceSearch.hpp
 *
 *  Created on: Oct 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_THREATSPACESEARCH_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_THREATSPACESEARCH_HPP_

#include <alphagomoku/networks/NNUE.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
#include <alphagomoku/search/alpha_beta/SharedHashTable.hpp>
#include <alphagomoku/search/alpha_beta/ThreatGenerator.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
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
	struct TSSStats
	{
			TimedStat setup;
			TimedStat static_solver;
			TimedStat hashtable;
			TimedStat move_generation;
			TimedStat solve;
			TimedStat evaluate;
			int64_t hits = 0;
			int64_t total_positions = 0;
			int64_t cache_hits = 0;
			int64_t cache_calls = 0;
			int64_t cache_colissions = 0;

			TSSStats();
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

	enum class TssMode
	{
		BASIC, /* only game end conditions are checked */
		STATIC, /* static solver and move generator is used */
		RECURSIVE, /* iterative deepening VCF search is used */
	};

	class ThreatSpaceSearch
	{
		private:
			ActionStack action_stack;

			int max_positions; // maximum number of positions that will be searched
			TssMode search_mode = TssMode::BASIC;

			int position_counter = 0;

			GameConfig game_config;
			PatternCalculator pattern_calculator;
			ThreatGenerator threat_generator;
			nnue::InferenceNNUE inference_nnue;

			SharedHashTable shared_table;
			HashKey128 hash_key;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;
			TSSStats stats;
		public:
			ThreatSpaceSearch(const GameConfig &gameConfig, const TSSConfig &tssConfig);

			void loadWeights(const nnue::NNUEWeights &weights);
			void increaseGeneration();
			void clear();
			int64_t getMemory() const noexcept;
			void solve(SearchTask &task, TssMode mode, int maxPositions);
			void tune(float speed);

			TSSStats getStats() const;
			void clearStats();
			void print_stats() const;
		private:
			Score recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions);
			bool is_move_legal(Move m) const noexcept;
			Score evaluate();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_THREATSPACESEARCH_HPP_ */
