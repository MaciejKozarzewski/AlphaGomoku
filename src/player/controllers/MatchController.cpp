/*
 * MatchController.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/MatchController.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	MatchController::MatchController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void MatchController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::SETUP)
		{
			time_manager.startTimer();
			start_search();
			state = ControllerState::SEARCH;
		}

		if (state == ControllerState::SEARCH)
		{
			Node root = search_engine.getInfo();
			if (time_manager.getElapsedTime() > time_manager.getTimeForTurn(engine_settings, root.getDepth(), root.getValue())
					or search_engine.isSearchFinished())
			{
				search_engine.stopSearch();
				time_manager.stopTimer();
				time_manager.resetTimer();
				state = ControllerState::GET_BEST_ACTION;
			}
			else
				return;
		}

		if (state == ControllerState::GET_BEST_ACTION)
		{
			search_engine.logSearchInfo();
			outputQueue.push(Message(MessageType::PLAIN_STRING, prepare_summary()));
//			Node node = search_engine.getInfo(std::vector<Move>());
//			if (node.isLeaf())
//			{ // rare case if there was not enough time to evaluate even a single node in the tree
//				outputQueue.push(Message(MessageType::BEST_MOVE, Move(0, 0)));
//				state = ControllerState::IDLE;
//				return;
//			}
//			node.sortEdges();
//			std::cout << node.toString() << '\n';
//			for (int i = 0; i < node.numberOfEdges(); i++)
//				std::cout << node.getEdge(i).toString() << '\n';
//
//			BestEdgeSelector selector;
//			Move move = selector.select(&node)->getMove();
			Move best_move = get_best_move();
			outputQueue.push(Message(MessageType::BEST_MOVE, best_move));

			if (engine_settings.isUsingAutoPondering() and not engine_settings.isInAnalysisMode())
			{
				matrix<Sign> board = search_engine.getBoard();
				Board::putMove(board, best_move);
				search_engine.setPosition(board, invertSign(best_move.sign));

				start_search();
				state = ControllerState::PONDERING;
			}
			else
				state = ControllerState::IDLE;
		}

		if (state == ControllerState::PONDERING)
		{
			if (search_engine.isSearchFinished())
				state = ControllerState::IDLE;
		}
	}
	/*
	 * private
	 */
	void MatchController::start_search()
	{
		Node root_node = search_engine.getInfo();
		initial_node_count = root_node.getVisits();

		SearchConfig cfg = engine_settings.getSearchConfig();
		search_engine.setEdgeSelector(PuctSelector(cfg.exploration_constant, engine_settings.getStyleFactor()));
		search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
		search_engine.startSearch();
	}
	Move MatchController::get_best_move() const
	{
		Node root_node = search_engine.getInfo();
		BestEdgeSelector selector;
		return selector.select(&root_node)->getMove();
	}
	std::string MatchController::prepare_summary() const
	{
		BestEdgeSelector selector;
		std::vector<Move> principal_variation;
		while (true)
		{
			Node node = search_engine.getInfo(principal_variation);
			if (node.isLeaf())
				break;
			Move m = selector.select(&node)->getMove();
			principal_variation.push_back(m);
		}

		Node root_node = search_engine.getInfo();

		std::string result;
		result += "depth 1-" + std::to_string(principal_variation.size());
		switch (root_node.getProvenValue())
		{
			case ProvenValue::UNKNOWN:
			{
				if (root_node.getVisits() > 0)
				{
					int tmp = static_cast<int>(1000 * (root_node.getWinRate() + 0.5f * root_node.getDrawRate()));
					result += " ev " + std::to_string(tmp / 10) + '.' + std::to_string(tmp % 10);
				}
				else
					result += " ev U"; // this should never happen, but just in case...
				break;
			}
			case ProvenValue::LOSS:
				result += " ev L";
				break;
			case ProvenValue::DRAW:
				result += " ev D";
				break;
			case ProvenValue::WIN:
				result += " ev W";
				break;
		}
		int64_t evaluated_nodes = root_node.getVisits() - initial_node_count;
		result += " n " + std::to_string(evaluated_nodes);
		if (evaluated_nodes > 0)
			result += " n/s " + std::to_string((int) (evaluated_nodes / time_manager.getLastSearchTime()));
		else
			result += " n/s 0";
		result += " tm " + std::to_string((int) (1000 * time_manager.getLastSearchTime()));
		result += " pv";

		for (size_t i = 0; i < principal_variation.size(); i++)
			result += " " + principal_variation[i].text();
		return result;
	}
} /* namespace ag */

