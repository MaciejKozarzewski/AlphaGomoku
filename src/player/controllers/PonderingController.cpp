/*
 * PonderingController.cpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/PonderingController.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	PonderingController::PonderingController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void PonderingController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::SETUP)
		{
			start_best_move_search();
			state = ControllerState::PONDERING;
		}

		if (state == ControllerState::PONDERING)
		{
			if (time_manager.getElapsedTime() > engine_settings.getTimeForPondering() or search_engine.isSearchFinished())
			{
				stop_search(false);
				search_engine.stopSearch();
				time_manager.stopTimer();
				time_manager.resetTimer();
				state = ControllerState::IDLE;
			}
		}
	}
} /* namespace ag */

