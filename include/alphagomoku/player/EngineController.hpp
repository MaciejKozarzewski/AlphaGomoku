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
		protected:
			const EngineSettings &engine_settings;
			TimeManager &time_manager;
			SearchEngine &search_engine;

		public:
			EngineController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			EngineController(const EngineController &other) = delete;
			EngineController(EngineController &&other) = delete;
			EngineController& operator=(const EngineController &other) = delete;
			EngineController& operator=(EngineController &&other) = delete;
			virtual ~EngineController();

			virtual void setup(const std::string &args);
			virtual void control(MessageQueue &outputQueue) = 0;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_ */
