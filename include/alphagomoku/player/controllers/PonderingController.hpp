/*
 * PonderingController.hpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_PONDERINGCONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_PONDERINGCONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>

namespace ag
{
	class PonderingController: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				SETUP,
				PONDERING
			};
			ControllerState state = ControllerState::SETUP;
		public:
			PonderingController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_PONDERINGCONTROLLER_HPP_ */
