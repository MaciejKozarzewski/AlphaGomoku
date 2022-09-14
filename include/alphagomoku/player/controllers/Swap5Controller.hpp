/*
 * Swap5Controller.hpp
 *
 *  Created on: Jul 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP5CONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP5CONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	/*
	 * \brief Controller for 'swap5' opening rule:
	 * - The black player puts the 1st move anywhere on board.
	 * - The other player may swap.
	 * - The white player puts the 2nd move anywhere on board.
	 * - The other player may swap.
	 * - The black player puts the 3rd move anywhere on board.
	 * - The other player may swap.
	 * - The white player puts the 4th move anywhere on board.
	 * - The other player may swap.
	 * - The black player puts the 5th move anywhere on board.
	 * - The other player may swap.
	 * - The white player plays the 6th move anywhere on board.
	 */
	class Swap5Controller: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				CHECK_BOARD,
				BALANCE,
				PLAY
			};
			ControllerState state = ControllerState::CHECK_BOARD;
			bool must_play = false;
		public:
			Swap5Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void setup(const std::string &args);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_SWAP5CONTROLLER_HPP_ */
