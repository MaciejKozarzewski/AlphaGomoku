/*
 * Search.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/search/alpha_beta/AlphaBetaSearch.hpp>
#include <alphagomoku/search/alpha_beta/VCFSolver.hpp>
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

			SearchStats();
			std::string toString() const;
			SearchStats& operator+=(const SearchStats &other) noexcept;
			SearchStats& operator/=(int i) noexcept;
			double getTotalTime() const noexcept;
	};

	class Search
	{
		private:
			SearchTaskList tasks_list_buffer[2];

			AlphaBetaSearch ab_search;
			VCFSolver vcf_solver;

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

			int current_task_buffer = 0;
		public:
			static constexpr int maximum_number_of_simulations = 16777216;
			Search(const GameConfig &gameOptions, const SearchConfig &searchOptions);

			int64_t getMemory() const noexcept;
			const SearchConfig& getConfig() const noexcept;
			AlphaBetaSearch& getSolver() noexcept;
			void clearStats() noexcept;
			SearchStats getStats() const noexcept;

			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void select(Tree &tree, int maxSimulations = maximum_number_of_simulations);
			void solve(double endTime = -1.0);
			void scheduleToNN(NNEvaluator &evaluator);
			bool areTasksReady() const noexcept;
			void generateEdges(const Tree &tree);
			void expand(Tree &tree);
			void backup(Tree &tree);
			void cleanup(Tree &tree);

			void useBuffer(int index) noexcept;
			void switchBuffer() noexcept;
			void setBatchSize(int batchSize) noexcept;
			int getBatchSize() const noexcept;
		private:
			const SearchTaskList& get_buffer() const noexcept;
			SearchTaskList& get_buffer() noexcept;
			bool is_duplicate(const SearchTask &task) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_SEARCH_HPP_ */
