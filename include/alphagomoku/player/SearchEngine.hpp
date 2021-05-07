/*
 * SearchEngine.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_

#include <alphagomoku/utils/ThreadPool.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/player/ResourceManager.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <libml/utils/json.hpp>

#include <iostream>

namespace ag
{

	class SearchThread: public Job
	{
		private:
			EvaluationQueue eval_queue;
			Search search;
			mutable std::mutex search_mutex;
			bool is_running = false;
		public:
			SearchThread(GameConfig gameConfig, const Json &cfg, Tree &tree, Cache &cache, ml::Device device);
			void setup(const matrix<Sign> &board);
			void stop() noexcept;
			bool isRunning() const noexcept;
			void run();

			SearchStats getSearchStats() const noexcept;
			QueueStats getQueueStats() const noexcept;
	};

	class SearchEngine
	{
		private:
			ResourceManager &resource_manager;
			ThreadPool thread_pool;
			Tree tree;
			Cache cache;
			std::vector<std::unique_ptr<SearchThread>> search_threads;

			matrix<Sign> board;
			Sign sign_to_move = Sign::NONE;
			Json config;
			double time_used_for_last_search = 0.0;
		public:
			SearchEngine(const Json &cfg, ResourceManager &rm);

			/**
			 * Used to setup entire board in one step. Moves must be in correct order (alternating colors)
			 */
			void setPosition(const std::vector<Move> &listOfMoves);
			/**
			 * Used to incrementally add moves to board. Moves must be in correct order (alternating colors)
			 */
			void setPosition(const Move &move);

			Message makeMove();
			Message ponder();
			Message swap2();
			Message swap();
			void exit();

			void stopSearch();
			int getSimulationCount() const;
			Message getSearchSummary();
			bool isSearchFinished() const noexcept;
		private:
			void setup_search();
			Move get_best_move() const;
			float get_root_eval() const;
			Message make_forced_move();
			Message make_move_by_search();
			Message make_move_by_network();
			Message swap2_0stones();
			Message swap2_3stones();
			Message swap2_5stones();
			bool search_continues(double timeout);
			void log_search_info();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_ */
