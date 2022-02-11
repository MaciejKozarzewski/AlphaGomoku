/*
 * MatchController.hpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_CONTROLLERS_MATCHCONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_CONTROLLERS_MATCHCONTROLLER_HPP_

#include <alphagomoku/player/EngineController.hpp>

namespace ag
{
	class MatchController: public EngineController
	{
		private:
			enum class ControllerState
			{
				IDLE,
				SETUP,
				SEARCH,
				GET_BEST_ACTION,
				PONDERING
			};
			ControllerState state = ControllerState::IDLE;
		public:
			MatchController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			void control(MessageQueue &outputQueue);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_CONTROLLERS_MATCHCONTROLLER_HPP_ */
