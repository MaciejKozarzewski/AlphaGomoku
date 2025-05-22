/*
 * MatchController.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/MatchController.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/protocols/Protocol.hpp>

#include <alphagomoku/utils/Logger.hpp>

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
			start_best_move_search();
			state = ControllerState::SEARCH;
		}

		if (state == ControllerState::SEARCH)
		{
			const int move_number = search_engine.getTree().getMoveNumber();
			const float moves_left = search_engine.getTree().getMovesLeft();
			const double time_for_turn = time_manager.getTimeForTurn(engine_settings, move_number, moves_left);
			if (is_search_completed(time_for_turn))
			{
				stop_search();
				state = ControllerState::GET_BEST_ACTION;
			}
			else
				return;
		}

		if (state == ControllerState::GET_BEST_ACTION)
		{
			SearchSummary summary = search_engine.getSummary( { }, true);
			summary.time_used = time_manager.getLastSearchTime();
			outputQueue.push(Message(MessageType::INFO_MESSAGE, summary));

			const Move best_move = get_best_move(summary);
			outputQueue.push(Message(MessageType::BEST_MOVE, best_move));

			if (engine_settings.isUsingAutoPondering() and not engine_settings.isInAnalysisMode())
			{
				matrix<Sign> board = search_engine.getTree().getBoard();
				Board::putMove(board, best_move);
				search_engine.setPosition(board, invertSign(best_move.sign));

				start_best_move_search();
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
} /* namespace ag */

