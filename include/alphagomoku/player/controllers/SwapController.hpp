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
	/*
	 * \brief Controller for 'swap' opening rule:
	 * - The first player puts 2 black and 1 white stones anywhere on the board.
	 * - The second player has 2 options:
	 * 		(a) stay with white,
	 * 		(b) swap,
	 */
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
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAPCONTROLLER_HPP_ */
