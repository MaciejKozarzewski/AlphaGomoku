/*
 * SearchEngine.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_

#include <alphagomoku/utils/ThreadPool.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <libml/utils/json.hpp>

#include <iostream>

namespace ag
{

	class SearchThread: public Job
	{
		private:
			EvaluationQueue eval_queue;
			Search_old search;
			mutable std::mutex search_mutex;
			bool is_running = false;
		public:
			SearchThread(GameConfig gameConfig, const Json &cfg, Tree_old &tree, Cache &cache, ml::Device device);
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
			EngineSettings settings;

			std::unique_ptr<Tree_old> tree;
			std::unique_ptr<Cache> cache;
			std::vector<std::unique_ptr<Search_old>> searchers;
			std::vector<std::unique_ptr<EvaluationQueue>> evaluators;

			matrix<Sign> board;
			Sign sign_to_move = Sign::NONE;
		public:
			/**
			 * Used to setup entire board in one step. Moves must be in correct order (alternating colors)
			 */
			void setPosition(const std::vector<Move> &listOfMoves);
			void startSearch();
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
