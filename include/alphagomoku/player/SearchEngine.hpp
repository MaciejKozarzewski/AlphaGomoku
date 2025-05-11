/*
 * SearchEngine.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_

#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>

#include <future>

namespace ag
{
	class EdgeSelector;
	class EdgeGenerator;
	class EngineSettings;
	class SearchSummary;
}

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
			void clearStats() noexcept;
	};

	class SearchEngine
	{
		private:
			const EngineSettings &settings;
			NNEvaluatorPool nn_evaluators;

			std::vector<std::unique_ptr<SearchThread>> search_threads;
			Tree tree;
		public:
			SearchEngine(const EngineSettings &settings);
			void reset();
			void setPosition(const matrix<Sign> &board, Sign signToMove);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			void startSearch();
			void stopSearch();
			bool isSearchFinished() const noexcept;
			bool isRootEvaluated() const noexcept;

			const Tree& getTree() const noexcept;
			void logSearchInfo() const;
			SearchSummary getSummary(const std::vector<Move> &listOfMoves, bool getPV) const;
			Node getRootCopy() const;
		private:
			void setup_search_threads();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_ */
