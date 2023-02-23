/*
 * Search.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cinttypes>
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
			TimedStat solve;
			TimedStat schedule;
			TimedStat generate;
			TimedStat expand;
			TimedStat backup;

			uint64_t nb_duplicate_nodes = 0;
			uint64_t nb_information_leaks = 0;
			uint64_t nb_wasted_expansions = 0;
			uint64_t nb_proven_states = 0;
			uint64_t nb_network_evaluations = 0;
			uint64_t nb_node_count = 0;
			uint64_t nb_batch_size = 0;

			SearchStats();
			std::string toString() const;
			SearchStats& operator+=(const SearchStats &other) noexcept;
			SearchStats& operator/=(int i) noexcept;
			double getTotalTime() const noexcept;
	};

	class Search
	{
		private:
			SearchTaskList tasks_list_buffer_0;
			SearchTaskList tasks_list_buffer_1;

			ThreatSpaceSearch solver;

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

			struct TuningPoint
			{
					double time = 0.0;
					uint64_t node_count = 0;
			};
			TuningPoint last_tuning_point;
			int current_task_buffer = 0;
		public:
			static constexpr int maximum_number_of_simulations = 16777216;
			Search(GameConfig gameOptions, SearchConfig searchOptions);

			int64_t getMemory() const noexcept;
			const SearchConfig& getConfig() const noexcept;
			ThreatSpaceSearch& getSolver() noexcept;
			void clearStats() noexcept;
			SearchStats getStats() const noexcept;

			void select(Tree &tree, int maxSimulations = maximum_number_of_simulations);
			void solve();
			void scheduleToNN(NNEvaluator &evaluator);
			bool areTasksReady() const noexcept;
			void generateEdges(const Tree &tree);
			void expand(Tree &tree);
			void backup(Tree &tree);
			void cleanup(Tree &tree);
			void tune();

			void useBuffer(int index) noexcept;
			void switchBuffer() noexcept;
			void setBatchSize(int batchSize) noexcept;
		private:
			const SearchTaskList& get_buffer() const noexcept;
			SearchTaskList& get_buffer() noexcept;
			bool is_duplicate(const SearchTask &task) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_ */
