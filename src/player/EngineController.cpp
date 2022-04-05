/*
 * EngineController.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	EngineController::EngineController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			engine_settings(settings),
			time_manager(manager),
			search_engine(engine)
	{
	}
	EngineController::~EngineController()
	{
		search_engine.stopSearch();
	}
	void EngineController::setup(const std::string &args)
	{
	}

} /* namespace ag */

