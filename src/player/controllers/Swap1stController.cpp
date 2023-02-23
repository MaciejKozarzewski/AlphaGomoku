/*
 * Swap1stController.cpp
 *
 *  Created on: Jul 1, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap1stController.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{
	Swap1stController::Swap1stController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void Swap1stController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::CHECK_BOARD)
		{
			switch (Board::numberOfMoves(search_engine.getBoard()))
			{
				case 0:
					Logger::write("Placing first stone");
					state = ControllerState::PUT_FIRST_STONE;
					break;
				case 1:
					Logger::write("Evaluating first stone");
					state = ControllerState::EVALUATE_FIRST_STONE;
					start_best_move_search();
					break;
			}
		}

		if (state == ControllerState::PUT_FIRST_STONE)
		{
			// TODO add loading first balanced move
//			const int r = randInt(engine_settings.getSwap1stOpenings().size());
//			Message msg(MessageType::BEST_MOVE, engine_settings.getSwap1stOpenings()[r]);
//			outputQueue.push(msg);
			state = ControllerState::IDLE;
			return;
		}

		if (state == ControllerState::EVALUATE_FIRST_STONE)
		{
			if (is_search_completed(time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				if (summary.node.getExpectation() < 0.5f)
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
				else
					outputQueue.push(Message(MessageType::BEST_MOVE, get_best_move(summary)));
				state = ControllerState::IDLE;
			}
		}
	}

} /* namespace ag */

