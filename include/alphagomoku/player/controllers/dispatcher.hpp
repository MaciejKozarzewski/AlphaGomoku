/*
 * dispatcher.hpp
 *
 *  Created on: Apr 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_DISPATCHER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_DISPATCHER_HPP_

#include <alphagomoku/player/EngineController.hpp>
#include <memory>

namespace ag
{
	std::unique_ptr<EngineController> createController(const std::string &type, const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_DISPATCHER_HPP_ */
