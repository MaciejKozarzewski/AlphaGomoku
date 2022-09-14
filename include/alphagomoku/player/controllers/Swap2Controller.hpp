/*
 * Swap2Controller.hpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	/*
	 * \brief Controller for 'swap2' opening rule:
	 * - The first player puts 2 black and 1 white stones anywhere on the board.
	 * - The second player has 3 options:
	 * 		(a) stay with white,
	 * 		(b) swap,
	 * 		(c) put 2 more stones and let the opponent choose the color
	 */
	class Swap2Controller: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				CHECK_BOARD,
				PUT_FIRST_3_STONES,
				EVALUATE_FIRST_3_STONES,
				BALANCE_THE_OPENING,
				EVALUATE_5_STONES
			};
			ControllerState state = ControllerState::CHECK_BOARD;
			Move first_balancing_move;
			Move second_balancing_move;
		public:
			Swap2Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP2CONTROLLER_HPP_ */
