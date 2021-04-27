/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/GomocupPlayer.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <iostream>
#include <fstream>
#include <string>

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

	SearchEngine::SearchEngine(GameConfig gameConfig, const Json &cfg) :
			thread_pool(cfg["threads"].size()),
			tree(cfg["tree_options"]),
			cache(gameConfig, cfg["cache_options"]),
			game_config(gameConfig),
			board(gameConfig.rows, gameConfig.cols),
			config(cfg)
	{
		for (int i = 0; i < thread_pool.size(); i++)
			search_threads.push_back(std::make_unique<SearchThread>(gameConfig, cfg, tree, cache, ml::Device::fromString(cfg["threads"][i]["device"])));
	}
	ResourceManager& SearchEngine::getResourceManager() noexcept
	{
		return resource_manager;
	}
	void SearchEngine::setPosition(const std::vector<Move> &listOfMoves)
	{
		if (listOfMoves.empty())
			sign_to_move = Sign::CROSS;
		else
			sign_to_move = invertSign(listOfMoves.back().sign);
		for (auto move = listOfMoves.begin(); move < listOfMoves.end(); move++)
			board.at(move->row, move->col) = move->sign;
	}
	Message SearchEngine::makeMove()
	{
		Message msg = make_forced_move();
		if (msg.isEmpty())
		{
			if (resource_manager.getTimeForTurn() == 0.0)
				return make_move_by_network();
			else
				return make_move_by_search();
		}
		else
			return msg;
	}
	void SearchEngine::ponder(double timeout)
	{
		setup_search();
		tree.setBalancingDepth(-1);
		while (search_continues(timeout))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();
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
	void SearchEngine::exit()
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->stop();
	}
//	Message SearchEngine::start_program(const Message &msg)
//	{
//		if (msg.getGameConfig().rows != msg.getGameConfig().cols)
//			return Message(MessageType::ERROR, "only square boards are supported");
//		if (msg.getGameConfig().rows != 15 and msg.getGameConfig().rows != 20)
//			return Message(MessageType::ERROR, "only 15x15 or 20x20 boards are supported");
//
//		game_config.rows = msg.getGameConfig().rows;
//		game_config.cols = msg.getGameConfig().cols;
////		return Message(MessageType::PLAIN_STRING, "OK");
//	}
//	Message SearchEngine::set_option(const Message &msg)
//	{
//		if (msg.getOption().name == "time_for_turn")
//			time_manager.setTimeForTurn(std::stod(msg.getOption().value) / 1000.0);
//		if (msg.getOption().name == "time_for_match")
//			time_manager.setTimeForMatch(std::stod(msg.getOption().value) / 1000.0);
//		if (msg.getOption().name == "time_left")
//			time_manager.setTimeLeft(std::stod(msg.getOption().value) / 1000.0);
//
//		if (msg.getOption().name == "max_memory")
//			max_memory = std::stoll(msg.getOption().value);
//		if (msg.getOption().name == "rules")
//			game_config.rules = rulesFromString(msg.getOption().value);
//		return Message();
//	}

	void SearchEngine::stopSearch()
	{
		for (size_t i = 0; i < search_threads.size(); i++)
			search_threads[i]->stop();
		thread_pool.waitForFinish();
		time_used_for_last_search = resource_manager.getElapsedTime();
	}
	int SearchEngine::getSimulationCount() const
	{
		return tree.getRootNode().getVisits() - 1;
	}
	Message SearchEngine::getSearchSummary()
	{
		std::string result;
		SearchTrajectory pv = tree.getPrincipalVariation();
		result += "depth 1-" + std::to_string(pv.length());
		int tmp = (int) (1000 * get_root_eval());
		result += " ev " + std::to_string(tmp / 10) + '.' + std::to_string(tmp % 10);
		result += " n " + std::to_string(getSimulationCount());
		result += " n/s " + std::to_string((int) (getSimulationCount() / time_used_for_last_search));
		result += " tm " + std::to_string((int) (1000 * time_used_for_last_search));
		result += " pv";

		char s = (sign_to_move == Sign::CROSS) ? 'X' : 'O';
		for (int i = 0; i < pv.length(); i++)
		{
			Node &current = pv.getNode(i);
			Move m(current.getMove());
//			if (player->config.is_using_yixin_board == true)
//				result += " " + std::string(1, s) + (char) (97 + m.row) + std::to_string((board.cols() - 1 - m.col));
//			else
			result += " " + std::string(1, s) + (char) (97 + m.row) + std::to_string((int) m.col);
			if (s == 'X')
				s = 'O';
			else
				s = 'X';
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
		stopSearch();
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
		matrix<float> policy(game_config.rows, game_config.cols);
		matrix<ProvenValue> proven_values(game_config.rows, game_config.cols);
		matrix<Value> action_values(game_config.rows, game_config.cols);
		tree.getPlayoutDistribution(tree.getRootNode(), policy);
		tree.getProvenValues(tree.getRootNode(), proven_values);
		tree.getActionValues(tree.getRootNode(), action_values);
		normalize(policy);

		Move most_visited, best_q;
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
			{
				if (proven_values.at(row, col) == ProvenValue::WIN)
					return Move(row, col, sign_to_move);
				if (proven_values.at(row, col) != ProvenValue::LOSS)
				{

				}
			}
		return Move();
	}
	float SearchEngine::get_root_eval() const
	{
		return 1.0f - MaxExpectation()(&(tree.getRootNode()));
	}
	Message SearchEngine::make_forced_move()
	{
		assert(getOutcome(game_config.rules, board) == GameOutcome::UNKNOWN);

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
					GameOutcome winner = getOutcome(game_config.rules, board, Move(i, j, sign_to_move));
					board_copy.at(i, j) = Sign::NONE;
					if ((sign_to_move == Sign::CROSS and winner == GameOutcome::CROSS_WIN)
							or (sign_to_move == Sign::CIRCLE and winner == GameOutcome::CIRCLE_WIN))
					{
						return Message(MessageType::MAKE_MOVE, Move(i, j, sign_to_move));
					}
				}

		// check for opp winning moves
		for (int i = 0; i < board.rows(); i++)
			for (int j = 0; j < board.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
				{
					board_copy.at(i, j) = invertSign(sign_to_move);
					GameOutcome winner = getOutcome(game_config.rules, board, Move(i, j, invertSign(sign_to_move)));
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
		setup_search();
		tree.setBalancingDepth(-1);
		while (search_continues(resource_manager.getTimeForTurn()))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();
	}
	Message SearchEngine::make_move_by_network()
	{

	}
	Message SearchEngine::swap2_0stones()
	{
		std::ifstream myfile(static_cast<std::string>(config["swap2_openings_file"]));
		if (myfile.is_open())
		{
			std::vector<std::string> openings;
			while (!myfile.eof())
			{
				std::string line;
				getline(myfile, line);
				if (line.size() > 0)
					openings.push_back(line);
			}
			myfile.close();
			return Message(MessageType::MAKE_MOVE, openings[randInt(openings.size())]);
		}
		else
			return Message(MessageType::ERROR, "No swap2 opening book found");
	}
	Message SearchEngine::swap2_3stones()
	{
		const double balancing_split = 0.25;
		const float swap2_evaluation_treshold = 0.6f;

		setup_search();
		tree.setBalancingDepth(-1);
		while (search_continues(resource_manager.getTimeForSwap2(3) * balancing_split))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();

		if (get_root_eval() < 1.0f - swap2_evaluation_treshold)
			return Message(MessageType::MAKE_MOVE, "swap");
		else
		{
			if (get_root_eval() > swap2_evaluation_treshold)
				return Message(MessageType::MAKE_MOVE, get_best_move());
			else
			{
				setup_search();
				tree.setBalancingDepth(2);
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
		setup_search();
		tree.setBalancingDepth(-1);
		while (search_continues(resource_manager.getTimeForSwap2(5)))
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		stopSearch();

		if (get_root_eval() < 0.5f)
			return Message(MessageType::MAKE_MOVE, "swap");
		else
			return Message(MessageType::MAKE_MOVE, get_best_move());
	}
	bool SearchEngine::search_continues(double timeout)
	{
		if (isSearchFinished())
			return false;
		else
		{
			if (getSimulationCount() < 1)
				return true;
			else
				return resource_manager.getElapsedTime() < timeout;
		}
	}

} /* namespace ag */

