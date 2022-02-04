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
			NNEvaluatorPool(const Json &config);
			void loadNetwork(const std::string &pathToNetwork);
			void unloadNetwork();
			NNEvaluator& get() const;
			void release(const NNEvaluator &queue) const;
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
			std::unique_ptr<Tree> tree;

		public:
			SearchEngine(const Json &config, const EngineSettings &settings);
			/**
			 * \brief Used to setup entire board in one step. Moves must be in the correct order (alternating colors)
			 */
			void setPosition(const matrix<Sign> &board, Sign signToMove);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			void startSearch();
			void stopSearch();
			bool isSearchFinished() const noexcept;

			Message getSearchSummary();

		private:
			Tree& get_tree();
			void setup_search_threads();

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
