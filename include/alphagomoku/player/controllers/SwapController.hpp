/*
 * SwapController.hpp
 *
 *  Created on: Apr 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAPCONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAPCONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>

namespace ag
{
	class SwapController: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				CHECK_BOARD,
				PUT_FIRST_3_STONES,
				EVALUATE_FIRST_3_STONES
			};
			ControllerState state = ControllerState::CHECK_BOARD;
		public:
			SwapController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
		private:
			void start_search();
			void stop_search();
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAPCONTROLLER_HPP_ */
