/*
 * SearchThread.cpp
 *
 *  Created on: Oct 25, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{
	SearchThread::SearchThread(const EngineSettings &settings, Tree &tree, const NNEvaluatorPool &evaluators) :
			settings(settings),
			tree(tree),
			evaluator_pool(evaluators),
			search(settings.getGameConfig(), settings.getSearchConfig())
	{
	}
	SearchThread::~SearchThread()
	{
		stop();
		join();
	}
	void SearchThread::start()
	{
		if (isRunning())
			throw std::logic_error("the search thread is already running");

		{ /* artificial scope for lock */
			std::lock_guard lock(search_mutex);
			is_running = true;
		}
		search_future = std::async(std::launch::async, [this]()
		{	this->run();});
	}
	void SearchThread::stop() noexcept
	{
		std::lock_guard lock(search_mutex);
		is_running = false;
	}
	void SearchThread::join() const
	{
		if (search_future.valid())
			search_future.wait();
	}
	bool SearchThread::isRunning() const noexcept
	{
		std::lock_guard lock(search_mutex);
		if (search_future.valid())
		{
			std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
			return search_status != std::future_status::ready;
		}
		else
			return false;
	}
	void SearchThread::run()
	{
		try
		{
			search.clearStats();

			{ /* artificial scope for lock */
				TreeLock lock(tree);
				if (isStopConditionFulfilled())
					return;
			}

			while (true)
			{
				{ /* artificial scope for lock */
					TreeLock lock(tree);
					search.select(tree);
				}
				NNEvaluator &evaluator = evaluator_pool.get();
				if(evaluator.isOnGPU())
				{ // run Ab search asynchronously to the network evaluation
					search.scheduleToNN(evaluator);
					evaluator.asyncEvaluateGraphLaunch();
					search.solve();
					evaluator.asyncEvaluateGraphJoin();
				}
				else
				{
					search.solve();
					search.scheduleToNN(evaluator);
					evaluator.evaluateGraph();
				}
				evaluator_pool.release(evaluator);

				search.generateEdges(tree); // this step doesn't require locking the tree
				{ /* artificial scope for lock */
					TreeLock lock(tree);
					search.expand(tree);
					search.backup(tree);
					if (isStopConditionFulfilled())
						break;
				}
				search.tune();
				std::lock_guard lock(search_mutex);
				if (is_running == false)
					break;
			}
			TreeLock lock(tree);
			search.cleanup(tree);
		} catch (std::exception &e)
		{
			Logger::write(std::string("SearchThread::run() threw ") + e.what());
		}
	}
	SearchStats SearchThread::getSearchStats() const noexcept
	{
		return search.getStats();
	}
	/*
	 * private
	 */
	bool SearchThread::isStopConditionFulfilled() const
	{
		// assuming tree is locked
		if (tree.getNodeCount() == 0)
			return false; // there must be at least one node (root node) in the tree
		if (tree.getMemory() >= settings.getMaxMemory())
		{
			Logger::write("Reached memory limit");
			return true;
		}
		if (tree.getNodeCount() >= settings.getMaxNodes())
		{
			Logger::write("Reached node count limit");
			return true;
		}
		if (tree.getMaximumDepth() >= settings.getMaxDepth())
		{
			Logger::write("Reached depth limit");
			return true;
		}
		if (tree.isProven())
		{
			Logger::write("Tree is proven");
			return true;
		}
		if (tree.hasSingleMove())
		{
			Logger::write("There is single move");
			return true;
		}
		return false;
	}

} /* namespace ag */

