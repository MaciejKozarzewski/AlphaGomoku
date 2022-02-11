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
			/**
			 * @brief Setup the time manager for general search.
			 */
			void resetTimer() noexcept;
			/**
			 * @brief Starts measuring elapsed time.
			 */
			void startTimer() noexcept;
			/**
			 * @brief Stops the timer.
			 * Is needed to be able to get elapsed time after the search ended.
			 */
			void stopTimer() noexcept;
			/**
			 * @brief Returns time spent on the current search.
			 */
			double getElapsedTime() const noexcept;

			double getLastSearchTime() const noexcept;

	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
