/*
 * Swap2Controller.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap2Controller.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	Swap2Controller::Swap2Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void Swap2Controller::control(MessageQueue &outputQueue)
	{

	}
} /* namespace ag */

