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
		state = ControllerState::SETUP;
	}
	void MatchController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::SETUP)
		{
			time_manager.startTimer();
			SearchConfig cfg = engine_settings.getSearchConfig();
			search_engine.setEdgeSelector(PuctSelector(cfg.exploration_constant, engine_settings.getStyleFactor()));
			search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
			search_engine.startSearch();
			state = ControllerState::SEARCH;
		}

		if (state == ControllerState::SEARCH)
		{
			if (time_manager.getElapsedTime() > 10.0 or search_engine.getMemory() > engine_settings.getMaxMemory())
			{
				search_engine.stopSearch();
				time_manager.stopTimer();
				time_manager.resetTimer();
				state = ControllerState::GET_BEST_ACTION;
			}
		}

		if (state == ControllerState::GET_BEST_ACTION)
		{
			Node node = search_engine.getInfo(std::vector<Move>());
			if (node.isLeaf())
			{ // rare case if there was not enough time to evaluate even a single node in the tree
				outputQueue.push(Message(MessageType::BEST_MOVE, Move(0, 0)));
				state = ControllerState::IDLE;
				return;
			}
			node.sortEdges();
			std::cout << node.toString() << '\n';
			for (int i = 0; i < node.numberOfEdges(); i++)
				std::cout << node.getEdge(i).toString() << '\n';

			BestEdgeSelector selector;
			Move move = selector.select(&node)->getMove();
			outputQueue.push(Message(MessageType::BEST_MOVE, move));
			if (engine_settings.isUsingAutoPondering())
			{
				matrix<Sign> board = search_engine.getBoard();
				Board::putMove(board, move);
				search_engine.setPosition(board, invertSign(move.sign));
				search_engine.startSearch();
				state = ControllerState::PONDERING;
			}
			else
				state = ControllerState::IDLE;
		}

		if (state == ControllerState::PONDERING)
		{
			if (search_engine.getMemory() > engine_settings.getMaxMemory())
			{
				search_engine.stopSearch();
				state = ControllerState::IDLE;
			}
		}
	}
} /* namespace ag */

