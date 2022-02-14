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
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <libml/utils/json.hpp>

#include <iostream>
#include <future>

namespace ag
{
	class NNEvaluatorPool
	{
		private:
			mutable std::vector<std::unique_ptr<NNEvaluator>> evaluators;
			mutable std::vector<int> free_evaluators;
			mutable std::mutex eval_mutex;
			mutable std::condition_variable eval_cond;
		public:
			NNEvaluatorPool(const EngineSettings &settings);
			NNEvaluator& get() const;
			void release(const NNEvaluator &queue) const;
			NNEvaluatorStats getStats() const noexcept;
	};

	class SearchThread
	{
		private:
			const EngineSettings &settings;
			Tree &tree;
			const NNEvaluatorPool &evaluator_pool;

			Search search;
			std::future<void> search_future;

			mutable std::mutex search_mutex;
			bool is_running = false;
		public:
			SearchThread(const EngineSettings &settings, Tree &tree, const NNEvaluatorPool &evaluators);
			void start();
			void stop() noexcept;
			void join() const;
			bool isRunning() const noexcept;
			void run();

			SearchStats getSearchStats() const noexcept;
		private:
			bool isStopConditionFulfilled() const;
	};

	class SearchEngine
	{
		private:
			const EngineSettings &settings;
			NNEvaluatorPool nn_evaluators;

			std::vector<std::unique_ptr<SearchThread>> search_threads;
			Tree tree;

			int64_t initial_node_count = 0;

		public:
			SearchEngine(const EngineSettings &settings);
			void setPosition(const matrix<Sign> &board, Sign signToMove);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			void startSearch();
			void stopSearch();
			bool isSearchFinished() const noexcept;

			const matrix<Sign>& getBoard() const noexcept;
			Sign getSignToMove() const noexcept;
			void logSearchInfo() const;
			SearchSummary getSummary(const std::vector<Move> &listOfMoves, bool getPV) const;
		private:
			void setup_search_threads();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_ */
