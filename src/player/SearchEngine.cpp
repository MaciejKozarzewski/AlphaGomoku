/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/protocols/Protocol.hpp>

#include <minml/utils/json.hpp>

#include <filesystem>
#include <string>
#include <limits>

namespace ag
{
	/*
	 * NNEvaluatorPool
	 */
	NNEvaluatorPool::NNEvaluatorPool(const EngineSettings &settings)
	{
		for (size_t i = 0; i < settings.getDeviceConfigs().size(); i++)
		{
			evaluators.push_back(std::make_unique<NNEvaluator>(settings.getDeviceConfigs().at(i)));
			evaluators.back()->useSymmetries(settings.isUsingSymmetries());
			evaluators.back()->loadGraph(settings.getPathToConvNetwork());
			free_evaluators.push_back(i);
		}
	}
	NNEvaluator& NNEvaluatorPool::get() const
	{
		std::unique_lock lock(eval_mutex);
		eval_cond.wait(lock, [this]() // @suppress("Invalid arguments")
				{	return free_evaluators.size() > 0;});

		const int idx = free_evaluators.back();
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

	/*
	 * SearchEngine
	 */
	SearchEngine::SearchEngine(const EngineSettings &settings) :
			settings(settings),
			nn_evaluators(settings),
			tree(settings.getSearchConfig().tree_config)
	{
	}
	void SearchEngine::reset()
	{
		tree.clear();
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->reset();
	}
	void SearchEngine::setPosition(const matrix<Sign> &board, Sign signToMove)
	{
		assert(isSearchFinished());
		TreeLock lock(tree);
		tree.setBoard(board, signToMove, true);
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->setPosition(board, signToMove);
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
		nn_evaluators.clearStats();
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
	bool SearchEngine::isRootEvaluated() const noexcept
	{
		return tree.getNodeCount() > 0;
	}
	const Tree& SearchEngine::getTree() const noexcept
	{
		return tree;
	}
	void SearchEngine::logSearchInfo() const
	{
		if (Logger::isEnabled() == false)
			return;

		TreeLock lock(tree);
		matrix<Sign> board = tree.getBoard();
		matrix<float> policy(board.rows(), board.cols());
		matrix<Score> action_acores(board.rows(), board.cols());

		Node root_node = tree.getInfo( { });
		for (int i = 0; i < root_node.numberOfEdges(); i++)
		{
			Move m = root_node.getEdge(i).getMove();
			policy.at(m.row, m.col) = (root_node.getVisits() > 1) ? root_node.getEdge(i).getVisits() : root_node.getEdge(i).getPolicyPrior();
			action_acores.at(m.row, m.col) = root_node.getEdge(i).getScore();
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
					if (action_acores.at(i, j).isProven())
						result += action_acores.at(i, j).toFormattedString();
					else
					{
						const int t = (int) (1000 * policy.at(i, j));
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
			result.number_of_nodes += search_threads[i]->getSearchStats().nb_node_count;
		return result;
	}
	/*
	 * private
	 */
	void SearchEngine::setup_search_threads()
	{
		if (settings.getThreadNum() > static_cast<int>(search_threads.size()))
		{
			const int num_to_add = settings.getThreadNum() - static_cast<int>(search_threads.size());
			for (int i = 0; i < num_to_add; i++)
				search_threads.push_back(std::make_unique<SearchThread>(settings, tree, nn_evaluators));
		}
		if (settings.getThreadNum() < static_cast<int>(search_threads.size()))
			search_threads.erase(search_threads.begin() + settings.getThreadNum(), search_threads.end());
	}

} /* namespace ag */

