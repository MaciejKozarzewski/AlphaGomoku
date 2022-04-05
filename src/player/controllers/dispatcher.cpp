/*
 * dispatcher.cpp
 *
 *  Created on: Apr 5, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/dispatcher.hpp>

#include <alphagomoku/player/controllers/MatchController.hpp>
#include <alphagomoku/player/controllers/PonderingController.hpp>
#include <alphagomoku/player/controllers/Swap2Controller.hpp>
#include <alphagomoku/player/controllers/SwapController.hpp>

#include <iostream>

namespace ag
{
	std::unique_ptr<EngineController> createController(const std::string &type, const EngineSettings &settings, TimeManager &manager,
			SearchEngine &engine)
	{
		if (type == "bestmove")
			return std::make_unique<MatchController>(settings, manager, engine);
		if (type == "ponder")
			return std::make_unique<PonderingController>(settings, manager, engine);
		if (type == "swap2")
			return std::make_unique<Swap2Controller>(settings, manager, engine);
		if (type == "swap")
			return std::make_unique<SwapController>(settings, manager, engine);
		return nullptr;
	}
} /* namespace ag */

