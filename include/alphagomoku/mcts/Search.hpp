/*
 * Search.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCH_HPP_
#define ALPHAGOMOKU_MCTS_SEARCH_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <inttypes.h>
#include <string>
#include <vector>

namespace ag
{
	class EvaluationQueue;
	class Cache;
	class Tree;
	class Tree_old;
} /* namespace ag */

namespace ag
{
	struct SearchStats
	{
			TimedStat select;
			TimedStat evaluate;
			TimedStat schedule;
			TimedStat expand;
			TimedStat backup;

			uint64_t nb_select = 0;
			uint64_t nb_expand = 0;
			uint64_t nb_vcf_solver = 0;
			uint64_t nb_backup = 0;
			uint64_t nb_evaluate = 0;
			uint64_t nb_game_rules = 0;
			uint64_t nb_duplicate_nodes = 0;
			uint64_t nb_information_leaks = 0;

			double time_select = 0.0;
			double time_expand = 0.0;
			double time_vcf_solver = 0.0;
			double time_backup = 0.0;
			double time_evaluate = 0.0;
			double time_game_rules = 0.0;

			std::string toString() const;
			SearchStats& operator+=(const SearchStats &other) noexcept;
			SearchStats& operator/=(int i) noexcept;
	};

	class Search
	{
		private:
			std::vector<SearchTask> search_tasks;
			size_t active_task_count = 0;
			std::vector<std::pair<uint16_t, float>> moves_to_add;

			FeatureExtractor vcf_solver;

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

		public:
			Search(GameConfig gameOptions, SearchConfig searchOptions);

			void clearStats() noexcept;
			void printSolverStats() const;
			SearchStats getStats() const noexcept;
			SearchConfig getConfig() const noexcept;

			void select(Tree &tree, int maxSimulations = std::numeric_limits<int>::max());
			void evaluate();
			void scheduleToNN(EvaluationQueue &queue);
			void expand(Tree &tree);
			void backup(Tree &tree);
		private:
			int get_batch_size(int simulation_count) const noexcept;
			SearchTask& get_next_task();
			void correct_information_leak(SearchTask &task) const;
			void check_if_terminal(SearchTask &task) const;
	};

	class Search_old
	{
		private:
			struct SearchRequest
			{
					SearchTrajectory_old trajectory;
					EvaluationRequest position;
					SearchRequest(int rows, int cols) :
							position(rows, cols)
					{
					}
			};

			std::vector<SearchRequest> search_buffer;
			std::vector<std::pair<uint16_t, float>> moves_to_add;

			Cache &cache;
			Tree_old &tree;
			EvaluationQueue &eval_queue;

			FeatureExtractor vcf_solver;
			matrix<Sign> current_board;
			Sign sign_to_move;
			int simulation_count = 0;

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

		public:
			Search_old(GameConfig gameOptions, SearchConfig searchOptions, Tree_old &tree, Cache &cache, EvaluationQueue &queue);

			void clearStats() noexcept;
			void printSolverStats() const;
			SearchStats getStats() const noexcept;
			SearchConfig getConfig() const noexcept;
			int getSimulationCount() const noexcept;

			void setBoard(const matrix<Sign> &board);
			matrix<Sign> getBoard() const noexcept;

			void handleEvaluation();
			bool simulate(int maxSimulations);
			void cleanup();

		private:
			int get_batch_size() const noexcept;
			SearchRequest& select();
			GameOutcome evaluateFromGameRules(EvaluationRequest &position);
			void evaluate(EvaluationRequest &position);
			bool expand(EvaluationRequest &position);
			void backup(SearchRequest &request);
			bool isDuplicate(EvaluationRequest &request);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCH_HPP_ */
