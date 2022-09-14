/*
 * Swap1stController.hpp
 *
 *  Created on: Jul 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef PLAYER_CONTROLLERS_SWAP1STCONTROLLER_HPP_
#define PLAYER_CONTROLLERS_SWAP1STCONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	/*
	 * \brief Controller for 'swap after 1st' opening rule:
	 * - The first player puts the 1st move on the board.
	 * - The other player has the right to swap.
	 */
	class Swap1stController: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				CHECK_BOARD,
				PUT_FIRST_STONE,
				EVALUATE_FIRST_STONE
			};
			ControllerState state = ControllerState::CHECK_BOARD;
		public:
			Swap1stController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* PLAYER_CONTROLLERS_SWAP1STCONTROLLER_HPP_ */
