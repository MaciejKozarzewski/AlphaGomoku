/*
 * SearchThread.hpp
 *
 *  Created on: Oct 25, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHTHREAD_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHTHREAD_HPP_

#include <alphagomoku/search/monte_carlo/Search.hpp>

#include <future>

namespace ag
{
	class EngineSettings;
	class Tree;
	class NNEvaluatorPool;
}

namespace ag
{
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
			~SearchThread();
			void start();
			void stop() noexcept;
			void join() const;
			bool isRunning() const noexcept;
			void run();

			SearchStats getSearchStats() const noexcept;
		private:
			bool isStopConditionFulfilled() const;
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHTHREAD_HPP_ */
