/*
 * PonderingController.cpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/PonderingController.hpp>
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
			time_manager.startTimer();
			SearchConfig cfg = engine_settings.getSearchConfig();
			search_engine.setEdgeSelector(PuctSelector(cfg.exploration_constant, engine_settings.getStyleFactor()));
			search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
			search_engine.startSearch();
			state = ControllerState::PONDERING;
		}

		if (state == ControllerState::PONDERING)
		{
			if (time_manager.getElapsedTime() > engine_settings.getTimeForPondering() or search_engine.isSearchFinished())
			{
				search_engine.stopSearch();
				time_manager.stopTimer();
				time_manager.resetTimer();
				state = ControllerState::IDLE;
			}
		}
	}
} /* namespace ag */

