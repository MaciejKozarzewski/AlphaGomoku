/*
 * RIFController.hpp
 *
 *  Created on: Jul 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_RIFCONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_RIFCONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	/*
	 * \brief Controller for 'RIF' opening rule:
	 * - The first player puts one of the 26 openings.
	 * - The next player has a right to swap.
	 * - The white player puts the 4th move anywhere on board.
	 * - The black player puts two 5th moves on the board (symmetrical moves not allowed).
	 * - The white player chooses one 5th from these offerings and plays the 6th move.
	 */
	class RIFController: public EngineController
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
			RIFController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void setup(const std::string &args);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_RIFCONTROLLER_HPP_ */
