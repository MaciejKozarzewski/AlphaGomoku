/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/SearchEngine.hpp>
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
	SearchThread::SearchThread(GameConfig gameConfig, const Json &cfg, Tree &tree, Cache &cache, ml::Device device) :
			eval_queue(),
			search(gameConfig, SearchConfig(cfg["search_options"]), tree, cache, eval_queue)
	{
		switch (gameConfig.rules)
		{
			case GameRules::FREESTYLE:
				eval_queue.loadGraph(cfg["networks"]["freestyle"], static_cast<int>(cfg["search_options"]["batch_size"]), device,
						static_cast<bool>(cfg["use_symmetries"]));
				break;
			case GameRules::STANDARD:
				eval_queue.loadGraph(cfg["networks"]["standard"], static_cast<int>(cfg["search_options"]["batch_size"]), device,
						static_cast<bool>(cfg["use_symmetries"]));
				break;
			default:
				break;
		}
	}
	void SearchThread::setup(const matrix<Sign> &board)
	{
		if (isRunning())
			throw std::logic_error("SearchThread::setup() called while search is running");
		search.setBoard(board);
		std::lock_guard lock(search_mutex);
		is_running = true;
	}
	void SearchThread::stop() noexcept
	{
		std::lock_guard lock(search_mutex);
		is_running = false;
	}
	bool SearchThread::isRunning() const noexcept
	{
		std::lock_guard lock(search_mutex);
		return is_running;
	}
	void SearchThread::run()
	{
		ml::Device::cpu().setNumberOfThreads(1);
		search.clearStats();
		while (isRunning())
		{
			bool flag = search.simulate(16777216);
			if (flag == false)
				stop();
			eval_queue.evaluateGraph();
			search.handleEvaluation();
		}
		search.cleanup();
	}

	SearchEngine::SearchEngine(const Json &cfg, ResourceManager &rm) :
			resource_manager(rm),
			thread_pool(cfg["threads"].size()),
			tree(cfg["tree_options"]),
			cache(rm.getGameConfig(), cfg["cache_options"]),
			config(cfg)
	{
		for (int i = 0; i < thread_pool.size(); i++)
			search_threads.push_back(
					std::make_unique<SearchThread>(rm.getGameConfig(), cfg, tree, cache, ml::Device::fromString(cfg["threads"][i]["device"])));
		Logger::write("Initialized in " + std::to_string(rm.getElapsedTime()) + " seconds");
	}
	void SearchEngine::setPosition(const std::vector<Move> &listOfMoves)
	{
		if (listOfMoves.empty())
			sign_to_move = Sign::CROSS;
		else
			sign_to_move = invertSign(listOfMoves.back().sign);
		board = matrix<Sign>(resource_manager.getGameConfig().rows, resource_manager.getGameConfig().cols);
		for (auto move = listOfMoves.begin(); move < listOfMoves.end(); move++)
			board.at(move->row, move->col) = move->sign;
	}
	void SearchEngine::setPosition(const Move &move)
	{
		if (isBoardEmpty(board))
			sign_to_move = Sign::CROSS;
		assert(move.sign == sign_to_move);
		board.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(move.sign);
	}
	Message SearchEngine::makeMove()
	{
		Message msg = make_forced_move();
		if (msg.isEmpty())
		{
//			if (resource_manager.getTimeForTurn() == 0.0)
//				return make_move_by_network();
//			else
			return make_move_by_search();
		}
		else
		{
			tree.clear();
			time_used_for_last_search = resource_manager.getElapsedTime();
			return msg;
		}
	}
	Message SearchEngine::ponder()
	{
		setup_search();
		tree.setBalancingDepth(-1);
		while (search_continues(resource_manager.getTimeForPondering()))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();
		return Message();
	}
	Message SearchEngine::swap2()
	{
		int placed_stones = std::count_if(board.begin(), board.end(), [](Sign s)
		{	return s != Sign::NONE;});

		switch (placed_stones)
		{
			case 0:
				return swap2_0stones();
			case 3:
				return swap2_3stones();
			case 5:
				return swap2_5stones();
			default:
				return Message(MessageType::ERROR, "incorrect number of stones for swap2");
		}
	}
	Message SearchEngine::swap()
	{
		int placed_stones = std::count_if(board.begin(), board.end(), [](Sign s)
		{	return s != Sign::NONE;});

		switch (placed_stones)
		{
			case 0:
				return swap2_0stones();
			case 3:
				return swap2_5stones();
			default:
				return Message(MessageType::ERROR, "incorrect number of stones for swap2");
		}
	}
	void SearchEngine::exit()
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->stop();
	}
	void SearchEngine::stopSearch()
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->stop();
		thread_pool.waitForFinish();
		time_used_for_last_search = resource_manager.getElapsedTime();
	}
	int SearchEngine::getSimulationCount() const
	{
		return tree.getRootNode().getVisits();
	}
	Message SearchEngine::getSearchSummary()
	{
		log_search_info();

		std::string result;
		SearchTrajectory pv = tree.getPrincipalVariation();
		result += "depth 1-" + std::to_string(pv.length());
		switch (tree.getRootNode().getProvenValue())
		{
			case ProvenValue::UNKNOWN:
			{
				if (getSimulationCount() > 0)
				{
					int tmp = static_cast<int>(1000 * get_root_eval());
					result += " ev " + std::to_string(tmp / 10) + '.' + std::to_string(tmp % 10);
				}
				else
					result += " ev U"; // forced move was played, there is no root node evaluation to show
				break;
			}
			case ProvenValue::LOSS:
				result += " ev W"; // root node value is for the other player, needs to be inverted
				break;
			case ProvenValue::DRAW:
				result += " ev D";
				break;
			case ProvenValue::WIN:
				result += " ev L"; // root node value is for the other player, needs to be inverted
				break;
		}
		result += " n " + std::to_string(getSimulationCount());
		if (getSimulationCount() > 0)
			result += " n/s " + std::to_string((int) (getSimulationCount() / time_used_for_last_search));
		else
			result += " n/s 0";
		result += " tm " + std::to_string((int) (1000 * time_used_for_last_search));
		result += " pv";

		for (int i = 1; i < pv.length(); i++)
		{
			Node &current = pv.getNode(i);
			Move m(current.getMove());
			result += std::string(" ") + ((m.sign == Sign::CROSS) ? "X" : "O");
//			if (player->config.is_using_yixin_board == true)
//				result += (char) (97 + m.row) + std::to_string((board.cols() - 1 - m.col));
//			else
			result += (char) (97 + m.row) + std::to_string((int) m.col);
		}
		return Message(MessageType::INFO_MESSAGE, result);
	}
	bool SearchEngine::isSearchFinished() const noexcept
	{
		return thread_pool.isReady();
	}
// private
	void SearchEngine::setup_search()
	{
		cache.clearStats();
		tree.clearStats();

		cache.cleanup(board);
		tree.clear();

		tree.getRootNode().setMove( { 0, 0, invertSign(sign_to_move) });
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->setup(board);
		for (size_t i = 0; i < search_threads.size(); i++)
			thread_pool.addJob(search_threads[i].get());
	}
	Move SearchEngine::get_best_move() const
	{
		const int rows = resource_manager.getGameConfig().rows;
		const int cols = resource_manager.getGameConfig().cols;
		matrix<float> playouts(rows, cols);
		matrix<float> policy_priors(rows, cols);
		matrix<ProvenValue> proven_values(rows, cols);
		matrix<Value> action_values(rows, cols);
		tree.getPlayoutDistribution(tree.getRootNode(), playouts);
		tree.getPolicyPriors(tree.getRootNode(), policy_priors);
		tree.getProvenValues(tree.getRootNode(), proven_values);
		tree.getActionValues(tree.getRootNode(), action_values);

		double best_value = std::numeric_limits<double>::lowest();
		Move best_move;
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
			{
				double value = playouts.at(row, col)
						+ (action_values.at(row, col).win + 0.5 * action_values.at(row, col).draw) * tree.getRootNode().getVisits()
						+ 0.001 * policy_priors.at(row, col)
						+ ((int) (proven_values.at(row, col) == ProvenValue::WIN) - (int) (proven_values.at(row, col) == ProvenValue::LOSS)) * 1.0e9;
				if (value > best_value and value != 0.0)
				{
					best_value = value;
					best_move = Move(row, col, sign_to_move);
				}
			}
		return best_move;
	}
	float SearchEngine::get_root_eval() const
	{
		return 1.0f - MaxExpectation()(&(tree.getRootNode()));
	}
	Message SearchEngine::make_forced_move()
	{
		const GameRules rules = resource_manager.getGameConfig().rules;
		assert(getOutcome(rules, board) == GameOutcome::UNKNOWN);

		// check for empty board first move
		if (isBoardEmpty(board) == true)
			return Message(MessageType::MAKE_MOVE, Move(board.rows() / 2, board.cols() / 2, sign_to_move));

		matrix<Sign> board_copy(board);
		// check for own winning moves
		for (int i = 0; i < board.rows(); i++)
			for (int j = 0; j < board.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
				{
					board_copy.at(i, j) = sign_to_move;
					GameOutcome winner = getOutcome(rules, board_copy, Move(i, j, sign_to_move));
					board_copy.at(i, j) = Sign::NONE;
					if ((sign_to_move == Sign::CROSS and winner == GameOutcome::CROSS_WIN)
							or (sign_to_move == Sign::CIRCLE and winner == GameOutcome::CIRCLE_WIN))
					{
						//return Message(MessageType::MAKE_MOVE, Move(i, j, sign_to_move));
						return Message(); // do not force winning move
					}
				}

		// check for opp winning moves
		for (int i = 0; i < board.rows(); i++)
			for (int j = 0; j < board.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
				{
					board_copy.at(i, j) = invertSign(sign_to_move);
					GameOutcome winner = getOutcome(rules, board_copy, Move(i, j, invertSign(sign_to_move)));
					board_copy.at(i, j) = Sign::NONE;
					if ((sign_to_move == Sign::CIRCLE and winner == GameOutcome::CROSS_WIN)
							or (sign_to_move == Sign::CROSS and winner == GameOutcome::CIRCLE_WIN))
					{
						return Message(MessageType::MAKE_MOVE, Move(i, j, sign_to_move));
					}
				}
		return Message();
	}
	Message SearchEngine::make_move_by_search()
	{
		tree.setBalancingDepth(-1);
		setup_search();
		while (search_continues(resource_manager.getTimeForTurn()))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();
		return Message(MessageType::MAKE_MOVE, get_best_move());
	}
	Message SearchEngine::swap2_0stones()
	{
		Logger::write("Placing 3 initial stones");
		if (std::filesystem::exists(static_cast<std::string>(config["swap2_openings_file"])) == false)
			return Message(MessageType::ERROR, "No swap2 opening book found");
		else
		{
			FileLoader fl(static_cast<std::string>(config["swap2_openings_file"]));
			Json json = fl.getJson();
			int r = randInt(json.size());
			std::vector<Move> moves;
			for (int i = 0; i < json[r].size(); i++)
				moves.push_back(Move(json[r][i]));
			return Message(MessageType::MAKE_MOVE, moves);
		}
	}
	Message SearchEngine::swap2_3stones()
	{
		const double balancing_split = 0.25;
		const float swap2_evaluation_treshold = 0.6f;

		Logger::write("Evaluating opening");
		tree.setBalancingDepth(-1);
		setup_search();
		while (search_continues(resource_manager.getTimeForSwap2(3) * balancing_split))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();

		if (get_root_eval() < 1.0f - swap2_evaluation_treshold)
			return Message(MessageType::MAKE_MOVE, "swap");
		else
		{
			if (get_root_eval() > swap2_evaluation_treshold)
				return Message(MessageType::MAKE_MOVE, std::vector<Move>( { get_best_move() }));
			else
			{
				if (getSimulationCount() > 0)
					log_search_info();

				Logger::write("Balancing opening");
				tree.setBalancingDepth(2);
				setup_search();
				while (search_continues(resource_manager.getTimeForSwap2(3) * (1.0 - balancing_split)))
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				stopSearch();

				SearchTrajectory st;
				tree.select(st, 0.0f);
				return Message(MessageType::MAKE_MOVE, std::vector<Move>( { st.getMove(1), st.getMove(2) }));
			}
		}
	}
	Message SearchEngine::swap2_5stones()
	{
		Logger::write("Evaluating opening");
		tree.setBalancingDepth(-1);
		setup_search();
		while (search_continues(resource_manager.getTimeForSwap2(5)))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();

		if (get_root_eval() < 0.5f)
			return Message(MessageType::MAKE_MOVE, "swap");
		else
			return Message(MessageType::MAKE_MOVE, std::vector<Move>( { get_best_move() }));
	}
	bool SearchEngine::search_continues(double timeout)
	{
		if (isSearchFinished())
			return false;
		else
		{
			if (getSimulationCount() < 10)
				return true;
			else
				return resource_manager.getElapsedTime() < timeout
						and (tree.getMemory() + cache.getMemory()) < 0.9 * (resource_manager.getMaxMemory() - 150 * 1024 * 1024);
		}
	}
	void SearchEngine::log_search_info()
	{
		const int rows = resource_manager.getGameConfig().rows;
		const int cols = resource_manager.getGameConfig().cols;

		if (getSimulationCount() == 0)
		{
			Move move = make_forced_move().getMove();
			matrix<float> policy(rows, cols);
			policy.at(move.row, move.col) = 0.999f;
			Logger::write(policyToString(board, policy));
			return;
		}
		int children = std::min(10, tree.getRootNode().numberOfChildren());
		tree.getRootNode().sortChildren();
		Logger::write("BEST");
		for (int i = 0; i < children; i++)
			Logger::write(tree.getRootNode().getChild(i).toString());

		matrix<float> policy(rows, cols);
		matrix<ProvenValue> proven_values(rows, cols);
		if (tree.getRootNode().getVisits() > 1)
			tree.getPlayoutDistribution(tree.getRootNode(), policy);
		else
			tree.getPolicyPriors(tree.getRootNode(), policy);
		tree.getProvenValues(tree.getRootNode(), proven_values);
		normalize(policy);

		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
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
			result += '\n';
		}
		Logger::write(result);
		SearchTrajectory st = tree.getPrincipalVariation();
		for (int i = 0; i < st.length(); i++)
			Logger::write(st.getNode(i).toString());

		Logger::write(tree.getStats().toString() + "memory = " + std::to_string(tree.getMemory() / 1048576) + "MB");
		Logger::write(cache.getStats().toString() + "memory = " + std::to_string(cache.getMemory() / 1048576) + "MB");
		Logger::write("");
	}

} /* namespace ag */

