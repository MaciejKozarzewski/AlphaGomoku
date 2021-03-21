/*
 * Search.hpp
 *
 *  Created on: Mar 20, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCH_HPP_
#define ALPHAGOMOKU_MCTS_SEARCH_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/utils/game_rules.hpp>

#include <inttypes.h>
#include <string>
#include <vector>

namespace ag
{
	class EvaluationQueue;
	class Cache;
	class Tree;
} /* namespace ag */

namespace ag
{
	struct SearchStats
	{
			uint64_t nb_cache_hits = 0;
			uint64_t nb_cache_calls = 0;
			uint64_t nb_select = 0;
			uint64_t nb_expand = 0;
			uint64_t nb_backup = 0;
			uint64_t nb_evaluate = 0;
			uint64_t nb_game_rules = 0;
			uint64_t nb_duplicate_nodes = 0;

			double time_select = 0.0;
			double time_expand = 0.0;
			double time_backup = 0.0;
			double time_evaluate = 0.0;
			double time_game_rules = 0.0;

			std::string toString() const;
			SearchStats& operator+=(const SearchStats &other);
	};

	struct SearchConfig
	{
			int rows;
			int cols;
			GameRules rules;
			int batch_size;
			float exploration_constant;
			float expansion_prior_treshold;
			float noise_weight;
			bool use_endgame_solver;
			bool augment_position;
	};

	class Search
	{
		private:
			struct SearchRequest
			{
					SearchTrajectory trajectory;
					EvaluationRequest position;
					SearchRequest(int rows, int cols) :
							position(rows, cols)
					{
					}
			};

			std::vector<SearchRequest> search_buffer;
			std::vector<std::pair<uint16_t, float>> moves_to_add;

			Cache &cache;
			Tree &tree;
			EvaluationQueue &eval_queue;

			matrix<Sign> current_board;
			Sign sign_to_move;
			int simulation_count = 0;

			SearchConfig config;
			SearchStats stats;

		public:
			Search(SearchConfig cfg, Tree &tree, Cache &cache, EvaluationQueue &queue);

			SearchStats getStats() const noexcept;
			SearchConfig getConfig() const noexcept;
			int getSimulationCount() const noexcept;

			void setBoard(const matrix<Sign> &board);

			void handleEvaluation();
			void iterate(int max_iterations);
			void cleanup();

			void clearStats() noexcept;
		private:
			SearchRequest& select();
			GameOutcome evaluateFromGameRules(EvaluationRequest &position);
			void evaluate(EvaluationRequest &position);
			void expand(EvaluationRequest &position);
			void backup(SearchRequest &request);
			bool isDuplicate(EvaluationRequest &request);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCH_HPP_ */
