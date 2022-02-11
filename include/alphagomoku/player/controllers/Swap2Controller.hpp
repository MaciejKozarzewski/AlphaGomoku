/*
 * Swap2Controller.hpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>

namespace ag
{
	class Swap2Controller: public EngineController
	{
		public:
			Swap2Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_ */
