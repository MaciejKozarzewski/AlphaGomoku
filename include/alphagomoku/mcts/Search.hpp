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
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/utils/configs.hpp>

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
			SearchStats& operator+=(const SearchStats &other) noexcept;
			SearchStats& operator/=(int i) noexcept;
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

			GameConfig game_config;
			SearchConfig search_config;
			SearchStats stats;

		public:
			Search(GameConfig gameOptions, SearchConfig searchOptions, Tree &tree, Cache &cache, EvaluationQueue &queue);

			void clearStats() noexcept;
			SearchStats getStats() const noexcept;
			SearchConfig getConfig() const noexcept;
			int getSimulationCount() const noexcept;

			void setBoard(const matrix<Sign> &board);
			matrix<Sign> getBoard() const noexcept;

			void handleEvaluation();
			bool simulate(int maxSimulations);
			void cleanup();

		private:
			SearchRequest& select();
			GameOutcome evaluateFromGameRules(EvaluationRequest &position);
			void evaluate(EvaluationRequest &position);
			bool expand(EvaluationRequest &position);
			void backup(SearchRequest &request);
			bool isDuplicate(EvaluationRequest &request);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCH_HPP_ */
