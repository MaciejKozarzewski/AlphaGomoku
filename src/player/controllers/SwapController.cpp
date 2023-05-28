/*
 * SwapController.cpp
 *
 *  Created on: Apr 5, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/SwapController.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/protocols/Protocol.hpp>

namespace ag
{
	SwapController::SwapController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void SwapController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::CHECK_BOARD)
		{
			switch (search_engine.getTree().getMoveNumber())
			{
				case 0:
					state = ControllerState::PUT_FIRST_3_STONES;
					break;
				case 3:
					state = ControllerState::EVALUATE_FIRST_3_STONES;
					start_best_move_search();
					break;
			}
		}

		if (state == ControllerState::PUT_FIRST_3_STONES)
		{
			Message msg;
			if (engine_settings.getSwap2Openings().size() == 0)
				msg = Message(MessageType::ERROR, "No swap2 opening book");
			else
			{
				const int r = randInt(engine_settings.getSwap2Openings().size());
				msg = Message(MessageType::BEST_MOVE, engine_settings.getSwap2Openings()[r]);
			}
			outputQueue.push(msg);
			state = ControllerState::IDLE;
		}

		if (state == ControllerState::EVALUATE_FIRST_3_STONES)
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
			else
				return;
		}
	}

} /* namespace ag */

