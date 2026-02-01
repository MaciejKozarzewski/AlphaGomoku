/*
 * VCFSolver.hpp
 *
 *  Created on: Aug 19, 2025
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_VCFSOLVER_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_VCFSOLVER_HPP_

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
	class AGNetwork;
}

namespace ag
{
	class VCFSolver
	{
		private:
			ActionStack action_stack;

			// search limits
			int max_depth = 100;
			int max_nodes = 1000;
			double max_time = std::numeric_limits<double>::max();
			int node_counter = 0;
			double start_time = 0.0;

			GameConfig game_config;
			PatternCalculator pattern_calculator;
			MoveGenerator move_generator;
			nnue::InferenceNNUE inference_nnue;
			nnue::TrainingNNUE_policy policy_nnue;

			std::unique_ptr<AGNetwork> network;
			matrix<float> policy;
			matrix<Value> action_values;

			SharedHashTable shared_table;
			HashKey128 hash_key;

			size_t total_positions = 0;
			size_t total_calls = 0;
			TimedStat total_time;
			TimedStat policy_time;
		public:
			VCFSolver(const GameConfig &gameConfig);
			void increaseGeneration();
			void clear();
			void loadWeights(const nnue::NNUEWeights &weights);
			int solve(SearchTask &task);
			void print_stats() const;
			int64_t getMemory() const noexcept;

			void setDepthLimit(int depth) noexcept;
			void setNodeLimit(int nodes) noexcept;
			void setTimeLimit(double time) noexcept;
		private:
			Score solve_attack(int depthRemaining, ActionList &actions);
			Score solve_defend(int depthRemaining, ActionList &actions);
			bool is_move_legal(Move m) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_VCFSOLVER_HPP_ */
