/*
 * SearchThread.cpp
 *
 *  Created on: Oct 25, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <alphagomoku/networks/NNUE.hpp>

namespace
{
	int get_batch_size(int simulation_count, int max_batch_size) noexcept
	{
		const int tmp = std::cbrt(simulation_count); // doubling batch size for every 8x increase of simulations count
		return std::max(1, std::min(max_batch_size, tmp));
	}
}

namespace ag
{
	SearchThread::SearchThread(const EngineSettings &settings, Tree &tree, const NNEvaluatorPool &evaluators) :
			settings(settings),
			tree(tree),
			evaluator_pool(evaluators),
			search(settings.getGameConfig(), settings.getSearchConfig())
	{
//		search.getSolver().loadWeights(nnue::NNUEWeights(settings.getPathToNnueNetwork())); // TODO return back to this once TSS gets improved
	}
	SearchThread::~SearchThread()
	{
		stop();
		join();
	}
	void SearchThread::reset()
	{
		search.getSolver().clear();
	}
	void SearchThread::setPosition(const matrix<Sign> &board, Sign signToMove)
	{
		assert(!isRunning());
		search.setBoard(board, signToMove);
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
			const std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
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

			NNEvaluator &evaluator = evaluator_pool.get();
			if (evaluator.isOnGPU())
				asynchronous_run(evaluator);
			else
				serial_run(evaluator);

			evaluator_pool.release(evaluator);

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
	void SearchThread::serial_run(NNEvaluator &evaluator)
	{
		double time_per_sample = 0.01;
		while (true)
		{
			{ /* artificial scope for lock */
				TreeLock lock(tree);
				const int batch_size = get_batch_size(tree.getSimulationCount(), search.getConfig().max_batch_size);
				search.setBatchSize(batch_size);
				search.select(tree);
			}
			search.solve(getTime() + 0.1 * search.getBatchSize() * time_per_sample);
			search.scheduleToNN(evaluator);
			time_per_sample = evaluator.evaluateGraph();

			search.generateEdges(tree); // this step doesn't require locking the tree
			{ /* artificial scope for lock */
				TreeLock lock(tree);
				search.expand(tree);
				search.backup(tree);
				if (isStopConditionFulfilled())
					break;
			}
			std::lock_guard lock(search_mutex);
			if (is_running == false)
				break;
		}
	}
	void SearchThread::asynchronous_run(NNEvaluator &evaluator)
	{
		search.useBuffer(0);
		double end_time = getTime() + 0.1;
		while (true)
		{
			search.generateEdges(tree); // this step doesn't require locking the tree
			{ /* artificial scope for lock */
				TreeLock lock(tree);
				search.expand(tree);
				search.backup(tree);
				if (isStopConditionFulfilled())
					break;

				const int batch_size = get_batch_size(tree.getSimulationCount(), search.getConfig().max_batch_size);
				search.setBatchSize(batch_size);
				search.select(tree);
			}
			search.solve(end_time);
			search.scheduleToNN(evaluator);
			evaluator.asyncEvaluateGraphJoin();
			end_time = evaluator.asyncEvaluateGraphLaunch();
			search.switchBuffer();

			std::lock_guard lock(search_mutex);
			if (is_running == false)
				break;
		}
		evaluator.asyncEvaluateGraphJoin();
	}
	bool SearchThread::isStopConditionFulfilled() const
	{
		// assuming tree is locked
		if (tree.getNodeCount() == 0)
			return false; // there must be at least one node (root node) in the tree
		if (tree.getSimulationCount() >= Search::maximum_number_of_simulations)
		{
			Logger::write("Reached maximum number of simulations");
			return true;
		}
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
		if (tree.isRootProven())
		{
			Logger::write("Root node is proven");
			return true;
		}
		if (tree.hasAllMovesProven())
		{
			Logger::write("All moves at root are proven");
			return true;
		}
		if (tree.hasSingleMove())
		{
			Logger::write("There is a single move");
			return true;
		}
		return false;
	}

} /* namespace ag */

