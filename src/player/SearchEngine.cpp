/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <libml/utils/json.hpp>

#include <filesystem>
#include <string>
#include <limits>

namespace ag
{
	NNEvaluatorPool::NNEvaluatorPool(const EngineSettings &settings)
	{
		for (size_t i = 0; i < settings.getDeviceConfigs().size(); i++)
		{
			evaluators.push_back(std::make_unique<NNEvaluator>(settings.getDeviceConfigs().at(i)));
			evaluators.back()->useSymmetries(settings.isUsingSymmetries());
			evaluators.back()->loadGraph(settings.getPathToNetwork());
			free_evaluators.push_back(i);
		}
	}
	NNEvaluator& NNEvaluatorPool::get() const
	{
		std::unique_lock lock(eval_mutex);
		eval_cond.wait(lock, [this]() // @suppress("Invalid arguments")
				{	return free_evaluators.size() > 0;});

		int idx = free_evaluators.back();
		free_evaluators.pop_back();
		return *(evaluators.at(idx));
	}
	void NNEvaluatorPool::release(const NNEvaluator &queue) const
	{
		std::lock_guard lock(eval_mutex);
		for (size_t i = 0; i < evaluators.size(); i++)
			if (evaluators.at(i).get() == &queue)
			{
				free_evaluators.push_back(i);
				eval_cond.notify_one();
				return;
			}
		throw std::logic_error("NNEvaluator to be released is not a member of this pool");
	}
	NNEvaluatorStats NNEvaluatorPool::getStats() const noexcept
	{
		std::lock_guard lock(eval_mutex);
		NNEvaluatorStats result;
		for (size_t i = 0; i < evaluators.size(); i++)
			result += evaluators[i]->getStats();
		return result;
	}
	void NNEvaluatorPool::clearStats() noexcept
	{
		std::lock_guard lock(eval_mutex);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->clearStats();
	}

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
				search.solve();

				NNEvaluator &evaluator = evaluator_pool.get();
				search.scheduleToNN(evaluator);
				evaluator.evaluateGraph();
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
		if (tree.hasSingleNonLosingMove())
		{
			Logger::write("There is single non-losing move");
			return true;
		}
		return false;
	}

	SearchEngine::SearchEngine(const EngineSettings &settings) :
			settings(settings),
			nn_evaluators(settings),
			tree(settings.getTreeConfig())
	{
	}
	void SearchEngine::setPosition(const matrix<Sign> &board, Sign signToMove)
	{
		assert(isSearchFinished());
		TreeLock lock(tree);
		tree.setBoard(board, signToMove);
	}
	void SearchEngine::setEdgeSelector(const EdgeSelector &selector)
	{
		assert(isSearchFinished());
		TreeLock lock(tree);
		tree.setEdgeSelector(selector);
	}
	void SearchEngine::setEdgeGenerator(const EdgeGenerator &generator)
	{
		assert(isSearchFinished());
		TreeLock lock(tree);
		tree.setEdgeGenerator(generator);
	}

	void SearchEngine::startSearch()
	{
		assert(isSearchFinished());
		setup_search_threads();
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->start();
	}
	void SearchEngine::stopSearch()
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->stop();
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->join();
	}
	bool SearchEngine::isSearchFinished() const noexcept
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			if (search_threads[i]->isRunning())
				return false;
		return true;
	}
	const matrix<Sign>& SearchEngine::getBoard() const noexcept
	{
		return tree.getBoard();
	}
	Sign SearchEngine::getSignToMove() const noexcept
	{
		return tree.getSignToMove();
	}
	void SearchEngine::logSearchInfo() const
	{
		if (Logger::isEnabled() == false)
			return;

		TreeLock lock(tree);
		matrix<Sign> board = getBoard();
		matrix<float> policy(board.rows(), board.cols());
		matrix<ProvenValue> proven_values(board.rows(), board.cols());

		Node root_node = tree.getInfo( { });
		for (int i = 0; i < root_node.numberOfEdges(); i++)
		{
			Move m = root_node.getEdge(i).getMove();
			policy.at(m.row, m.col) = (root_node.getVisits() > 1) ? root_node.getEdge(i).getVisits() : root_node.getEdge(i).getPolicyPrior();
			proven_values.at(m.row, m.col) = root_node.getEdge(i).getProvenValue();
		}
		normalize(policy);

		Logger::write(root_node.toString());
		root_node.sortEdges();
		Logger::write("BEST");
		const int children = std::min(10, root_node.numberOfEdges());
		for (int i = 0; i < children; i++)
			Logger::write(root_node.getEdge(i).getMove().toString() + " : " + root_node.getEdge(i).toString());

		std::string result = "  ";
		for (int i = 0; i < board.cols(); i++)
			result += std::string("  ") + static_cast<char>(static_cast<int>('a') + i) + " ";
		result += '\n';
		for (int i = 0; i < board.rows(); i++)
		{
			if (i < 10)
				result += " ";
			result += std::to_string(i);
			for (int j = 0; j < board.cols(); j++)
			{
				if (board.at(i, j) == Sign::NONE)
				{
					switch (proven_values.at(i, j))
					{
						case ProvenValue::UNKNOWN:
						{
							int t = (int) (1000 * policy.at(i, j));
							if (t == 0)
								result += "  _ ";
							else
							{
								if (t < 1000)
									result += ' ';
								if (t < 100)
									result += ' ';
								if (t < 10)
									result += ' ';
								result += std::to_string(t);
							}
							break;
						}
						case ProvenValue::LOSS:
							result += " >L<";
							break;
						case ProvenValue::DRAW:
							result += " >D<";
							break;
						case ProvenValue::WIN:
							result += " >W<";
							break;
					}
				}
				else
					result += (board.at(i, j) == Sign::CROSS) ? "  X " : "  O ";
			}
			if (i < 10)
				result += " ";
			result += " " + std::to_string(i) + '\n';
		}
		result += "  ";
		for (int i = 0; i < board.cols(); i++)
			result += std::string("  ") + static_cast<char>(static_cast<int>('a') + i) + " ";
		result += '\n';
		Logger::write(result);

		BestEdgeSelector selector;
		std::vector<Move> principal_variation;
		while (true)
		{
			Node node = tree.getInfo(principal_variation);
			if (node.isLeaf())
				break;
			Edge *edge = selector.select(&node);
			Move m = edge->getMove();
			principal_variation.push_back(m);
			Logger::write(m.toString() + " : " + edge->toString());
		}

		NNEvaluatorStats evaluator_stats = nn_evaluators.getStats();
		SearchStats search_stats;
		for (size_t i = 0; i < search_threads.size(); i++)
			search_stats += search_threads[i]->getSearchStats();

		Logger::write("\n" + evaluator_stats.toString());
		Logger::write(search_stats.toString());
		Logger::write(tree.getNodeCacheStats().toString());
		Logger::write("memory usage = " + std::to_string(tree.getMemory() / 1048576) + "MB");
		Logger::write("");
	}
	SearchSummary SearchEngine::getSummary(const std::vector<Move> &listOfMoves, bool getPV) const
	{
		TreeLock lock(tree);
		SearchSummary result;
		result.node = tree.getInfo(listOfMoves);

		if (getPV)
		{
			result.principal_variation = listOfMoves;
			BestEdgeSelector selector;
			while (true)
			{
				Node node = tree.getInfo(result.principal_variation);
				if (node.isLeaf())
					break;
				Move m = selector.select(&node)->getMove();
				result.principal_variation.push_back(m);
			}
			result.principal_variation.erase(result.principal_variation.begin(), result.principal_variation.begin() + listOfMoves.size());
		}
		for (size_t i = 0; i < search_threads.size(); i++)
			result.number_of_nodes += search_threads[i]->getSearchStats().evaluate.getTotalCount();
		return result;
	}
	/*
	 * private
	 */
	void SearchEngine::setup_search_threads()
	{
		if (settings.getThreadNum() > static_cast<int>(search_threads.size()))
		{
			int num_to_add = settings.getThreadNum() - static_cast<int>(search_threads.size());
			for (int i = 0; i < num_to_add; i++)
				search_threads.push_back(std::make_unique<SearchThread>(settings, tree, nn_evaluators));
		}
		if (settings.getThreadNum() < static_cast<int>(search_threads.size()))
			search_threads.erase(search_threads.begin() + settings.getThreadNum(), search_threads.end());
	}

} /* namespace ag */

