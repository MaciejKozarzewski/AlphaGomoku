/*
 * Search.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCH_HPP_
#define ALPHAGOMOKU_MCTS_SEARCH_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/vcf_solver/VCFSolver.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>

#include <inttypes.h>
#include <string>
#include <vector>

namespace ag
{
	class EdgeSelector;
	class EdgeGenerator;
	class NNEvaluator;
	class Tree;
} /* namespace ag */

namespace ag
{
	struct SearchStats
	{
			TimedStat select;
			TimedStat evaluate;
			TimedStat schedule;
			TimedStat generate;
			TimedStat expand;
			TimedStat backup;

			uint64_t nb_duplicate_nodes = 0;
			uint64_t nb_information_leaks = 0;
			uint64_t nb_wasted_expansions = 0;
			uint64_t nb_network_evaluations = 0;
			uint64_t nb_node_count = 0;

			SearchStats();
			std::string toString() const;
			SearchStats& operator+=(const SearchStats &other) noexcept;
			SearchStats& operator/=(int i) noexcept;
			double getTotalTime() const noexcept;
	};

	class Search
	{
		private:
			std::vector<SearchTask> search_tasks;
			int active_task_count = 0;

			VCFSolver vcf_solver;

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

			struct TuningPoint
			{
					double time = 0.0;
					uint64_t node_count = 0;
			};

			TuningPoint last_tuning_point;
		public:
			Search(GameConfig gameOptions, SearchConfig searchOptions);

			const SearchConfig& getConfig() const noexcept;
			void clearStats() noexcept;
			SearchStats getStats() const noexcept;

			void select(Tree &tree, int maxSimulations = std::numeric_limits<int>::max());
			void solve();
			void scheduleToNN(NNEvaluator &evaluator);
			void generateEdges(const Tree &tree);
			void expand(Tree &tree);
			void backup(Tree &tree);
			void cleanup(Tree &tree);
			void tune();
		private:
			int get_batch_size(int simulation_count) const noexcept;
			SearchTask& get_next_task();
			bool is_duplicate(const SearchTask &task) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCH_HPP_ */
