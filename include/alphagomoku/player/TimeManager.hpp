/*
 * TimeManager.hpp
 *
 *  Created on: Sep 5, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_
#define ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_

#include <mutex>

namespace ag
{
	class EngineSettings;
	struct Value;
} /* namespace ag */

namespace ag
{
	enum class OpeningType
	{

	};

	class TimeManager
	{
		private:
			static constexpr double TIME_FRACTION = 0.04; /**< fraction of the remaining time that is used every move */
			static constexpr double SWAP2_FRACTION = 0.1;

			mutable std::mutex mutex;

			double start_time = 0.0; /**< [seconds] */
			double stop_time = 0.0; /**< [seconds] */
			double used_time = 0.0; /**< [seconds] */

			double time_of_last_search = 0.0; /**< [seconds] */
			bool is_running = false;
		public:
			void resetTimer() noexcept;
			void startTimer() noexcept;
			void stopTimer() noexcept;
			double getElapsedTime() const noexcept;
			double getLastSearchTime() const noexcept;

			double getTimeForTurn(const EngineSettings &settings, int moveNumber, Value eval);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
