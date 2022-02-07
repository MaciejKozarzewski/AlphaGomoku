/*
 * EngineController.hpp
 *
 *  Created on: Feb 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	class EngineSettings;
	class TimeManager;
	class SearchEngine;
	class MessageQueue;
}

namespace ag
{

	class EngineController
	{
		private:
			const EngineSettings &engine_settings;
			const TimeManager &time_manager;
			SearchEngine &search_engine;

		public:
			EngineController(const EngineSettings &settings, const TimeManager &manager, SearchEngine &engine) :
					engine_settings(settings),
					time_manager(manager),
					search_engine(engine)
			{
			}
			virtual ~EngineController() = 0;
			virtual void setPosition(const matrix<Sign> &board, Sign signToMove);
			virtual bool run(MessageQueue &output_queue) = 0;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_ */
