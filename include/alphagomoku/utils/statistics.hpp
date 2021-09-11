/*
 * statistics.hpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_STATISTICS_HPP_
#define ALPHAGOMOKU_UTILS_STATISTICS_HPP_

#include <inttypes.h>
#include <string>

namespace ag
{
	class TimedStat
	{
		private:
			std::string m_name;
			double m_timer_start = 0.0;
			double m_total_time = 0.0;
			uint64_t m_total_count = 0;
			bool m_is_measuring = false;
			bool m_is_paused = false;
		public:
			TimedStat() = default;
			TimedStat(const std::string &name);
			std::string getName() const;
			double getTimerStart() const noexcept;
			double getTotalTime() const noexcept;
			uint64_t getTotalCount() const noexcept;
			bool isMeasuring() const noexcept;
			bool isPaused() const noexcept;

			/**
			 * @brief Reset all collected measurements.
			 */
			void reset() noexcept;
			/**
			 * @brief Start time measurement.
			 */
			void startTimer() noexcept;
			/**
			 * @brief Pause current time measurement.
			 * Similar to @see stopTimer() but it does not increase total counter.
			 */
			void pauseTimer() noexcept;
			/**
			 * @brief Resume the time measurement, paused by @see pauseTimer() call.
			 */
			void resumeTimer() noexcept;
			/**
			 * @brief Stop current time measurement and increase the total counter.
			 */
			void stopTimer() noexcept;
			std::string toString() const;

			/**
			 * @brief Used to sum statistics over several instances of this class.
			 */
			TimedStat& operator+=(const TimedStat &other) noexcept;
			/**
			 * @brief Used to average statistics over several instances of this class.
			 */
			TimedStat& operator/=(int i) noexcept;
	};

	/**
	 * @brief Class that starts timer in constructor and automatically handles stopping of timer in destructor.
	 */
	class TimerGuard
	{
		private:
			TimedStat &m_stat;
		public:
			TimerGuard(TimedStat &stat) noexcept :
					m_stat(stat)
			{
				stat.startTimer();
			}
			~TimerGuard() noexcept
			{
				m_stat.stopTimer();
			}
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_STATISTICS_HPP_ */
