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
#include <iostream>
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
		result /= evaluators.size();
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
			search.tryToSolve();

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
			std::lock_guard lock(search_mutex);
			if (is_running == false)
				break;
		}
		TreeLock lock(tree);
		search.cleanup(tree);
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
		int children = std::min(10, root_node.numberOfEdges());
		for (int i = 0; i < children; i++)
			Logger::write(root_node.getEdge(i).toString());

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
		search_stats /= search_threads.size();

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

//	Move SearchEngine::get_best_move() const
//	{
//		const int rows = 0; //resource_manager.getGameConfig().rows;
//		const int cols = 0; //resource_manager.getGameConfig().cols;
//		matrix<float> playouts(rows, cols);
//		matrix<float> policy_priors(rows, cols);
//		matrix<ProvenValue> proven_values(rows, cols);
//		matrix<Value> action_values(rows, cols);
//		tree.getPlayoutDistribution(tree.getRootNode(), playouts);
//		tree.getPolicyPriors(tree.getRootNode(), policy_priors);
//		tree.getProvenValues(tree.getRootNode(), proven_values);
//		tree.getActionValues(tree.getRootNode(), action_values);
//
//		double best_value = std::numeric_limits<double>::lowest();
//		Move best_move;
//		for (int row = 0; row < board.rows(); row++)
//			for (int col = 0; col < board.cols(); col++)
//		{
//				double value = playouts.at(row, col)
//						+ (action_values.at(row, col).win + 0.5 * action_values.at(row, col).draw) * tree.getRootNode().getVisits()
//						+ 0.001 * policy_priors.at(row, col)
//						+ ((int) (proven_values.at(row, col) == ProvenValue::WIN) - (int) (proven_values.at(row, col) == ProvenValue::LOSS)) * 1.0e9;
//				if (value > best_value and value != 0.0)
//				{
//					best_value = value;
//					best_move = Move(row, col, sign_to_move);
//				}
//		}
//		return best_move;
//	}
//	float SearchEngine::get_root_eval() const
//	{
//		return 1.0f - MaxExpectation()(&(tree.getRootNode()));
//	}
//	Message SearchEngine::make_forced_move()
//	{
//		const GameRules rules = GameRules::FREESTYLE; //resource_manager.getGameConfig().rules;
//		assert(getOutcome(rules, board) == GameOutcome::UNKNOWN);
//
// check for empty board first move
//		if (isBoardEmpty(board) == true)
//			return Message(MessageType::BEST_MOVE, Move(board.rows() / 2, board.cols() / 2, sign_to_move));
//
//		matrix<Sign> board_copy(board);
// check for own winning moves
//		for (int i = 0; i < board.rows(); i++)
//			for (int j = 0; j < board.cols(); j++)
//				if (board.at(i, j) == Sign::NONE)
//				{
//					board_copy.at(i, j) = sign_to_move;
//					GameOutcome winner = getOutcome(rules, board_copy, Move(i, j, sign_to_move));
//					board_copy.at(i, j) = Sign::NONE;
//					if ((sign_to_move == Sign::CROSS and winner == GameOutcome::CROSS_WIN)
//							or (sign_to_move == Sign::CIRCLE and winner == GameOutcome::CIRCLE_WIN))
//					{
//						//return Message(MessageType::MAKE_MOVE, Move(i, j, sign_to_move));
//						return Message(); // do not force winning move
//					}
//				}
//
// check for opp winning moves
//		for (int i = 0; i < board.rows(); i++)
//			for (int j = 0; j < board.cols(); j++)
//				if (board.at(i, j) == Sign::NONE)
//				{
//					board_copy.at(i, j) = invertSign(sign_to_move);
//					GameOutcome winner = getOutcome(rules, board_copy, Move(i, j, invertSign(sign_to_move)));
//					board_copy.at(i, j) = Sign::NONE;
//					if ((sign_to_move == Sign::CIRCLE and winner == GameOutcome::CROSS_WIN)
//							or (sign_to_move == Sign::CROSS and winner == GameOutcome::CIRCLE_WIN))
//					{
//						return Message(MessageType::BEST_MOVE, Move(i, j, sign_to_move));
//					}
//				}
//		return Message();
//	}
//	Message SearchEngine::make_move_by_search()
//	{
//		tree.setBalancingDepth(-1);
//		setup_search();
//		while (search_continues(resource_manager.getTimeForTurn()))
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		stopSearch();
//		return Message(MessageType::BEST_MOVE, get_best_move());
//	}
//	Message SearchEngine::swap2_0stones()
//	{
//		Logger::write("Placing 3 initial stones");
//		if (std::filesystem::exists(static_cast<std::string>(config["swap2_openings_file"])) == false)
//			return Message(MessageType::ERROR, "No swap2 opening book found");
//		else
//		{
//			FileLoader fl(static_cast<std::string>(config["swap2_openings_file"])); FIXME later switch to using text format - "Xa0"
//			Json json = fl.getJson();
//			int r = randInt(json.size());
//			std::vector<Move> moves;
//			for (int i = 0; i < json[r].size(); i++)
//				moves.push_back(Move(json[r][i]));
//			return Message(MessageType::BEST_MOVE, moves);
//		}
//	}
//	Message SearchEngine::swap2_3stones()
//	{
//		const double balancing_split = 0.25;
//		const float swap2_evaluation_treshold = 0.6f;
//
//		Logger::write("Evaluating opening");
//		tree.setBalancingDepth(-1);
//		setup_search();
//		while (search_continues(resource_manager.getTimeForSwap2(3) * balancing_split))
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		stopSearch();
//
//		if (get_root_eval() < 1.0f - swap2_evaluation_treshold)
//			return Message(MessageType::BEST_MOVE, "swap");
//		else
//		{
//			if (get_root_eval() > swap2_evaluation_treshold)
//				return Message(MessageType::BEST_MOVE, std::vector<Move>( { get_best_move() }));
//			else
//			{
//				if (getSimulationCount() > 0)
//					log_search_info();
//
//				Logger::write("Balancing opening");
//				tree.setBalancingDepth(2);
//				setup_search();
//				while (search_continues(resource_manager.getTimeForSwap2(3) * (1.0 - balancing_split)))
//					std::this_thread::sleep_for(std::chrono::milliseconds(10));
//				stopSearch();

//				SearchTrajectory st;
//				tree.select(st, 0.0f);
//				if (st.length() < 2) // there was not enough time for balancing, just pick color
//				{
//					if (get_root_eval() < 0.5f)
//						return Message(MessageType::BEST_MOVE, "swap");
//					else
//						return Message(MessageType::BEST_MOVE, std::vector<Move>( { get_best_move() }));
//				}
//				else
//					return Message(MessageType::BEST_MOVE, std::vector<Move>( { st.getMove(1), st.getMove(2) }));
//			}
//		}
//	}
//	Message SearchEngine::swap2_5stones()
//	{
//		Logger::write("Evaluating opening");
//		tree.setBalancingDepth(-1);
//		setup_search();
//		while (search_continues(resource_manager.getTimeForSwap2(5)))
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
//		stopSearch();
//
//		if (get_root_eval() < 0.5f)
//			return Message(MessageType::BEST_MOVE, "swap");
//		else
//			return Message(MessageType::BEST_MOVE, std::vector<Move>( { get_best_move() }));
//	}
//	bool SearchEngine::search_continues(double timeout)
//	{
//		if (isSearchFinished())
//			return false;
//		else
//		{
//			if (getSimulationCount() < 10)
//				return true;
//			else
//				return resource_manager.getElapsedTime() < timeout
//						and (tree.getMemory() + cache.getMemory()) < 0.95 * (resource_manager.getMaxMemory() - 300 * 1024 * 1024);
//		}
//	}

} /* namespace ag */

