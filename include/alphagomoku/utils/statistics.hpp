/*
 * statistics.hpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_STATISTICS_HPP_
#define ALPHAGOMOKU_UTILS_STATISTICS_HPP_

#include <inttypes.h>
#include <chrono>
#include <cassert>
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
			TimedStat(const std::string &name) :
					m_name(name)
			{
			}
			std::string getName() const
			{
				return m_name;
			}
			double getTimerStart() const noexcept
			{
				return m_timer_start;
			}
			double getTotalTime() const noexcept
			{
				return m_total_time;
			}
			uint64_t getTotalCount() const noexcept
			{
				return m_total_count;
			}
			bool isMeasuring() const noexcept
			{
				return m_is_measuring;
			}
			bool isPaused() const noexcept
			{
				return m_is_paused;
			}

			static double getTime() noexcept
			{
				return std::chrono::steady_clock::now().time_since_epoch().count() * 1.0e-9;
			}

			/**
			 *\brief Reset all collected measurements.
			 */
			void reset() noexcept
			{
				m_is_measuring = false;
				m_is_paused = false;
				m_total_time = 0.0;
				m_total_count = 0;
			}
			/**
			 * \brief Start time measurement.
			 */
			void startTimer() noexcept
			{
				assert(isMeasuring() == false);
				m_is_measuring = true;
				m_timer_start = getTime();
			}
			/**
			 * \brief Pause current time measurement.
			 * Similar to @see stopTimer() but it does not increase total counter.
			 */
			void pauseTimer() noexcept
			{
				assert(isMeasuring());
				m_total_time += getTime() - m_timer_start;
				m_is_paused = true;
			}
			/**
			 * \brief Resume the time measurement, paused by @see pauseTimer() call.
			 */
			void resumeTimer() noexcept
			{
				assert(isMeasuring() && isPaused());
				m_timer_start = getTime();
				m_is_paused = false;
			}
			/**
			 * \brief Stop current time measurement and increase the total counter.
			 */
			void stopTimer() noexcept
			{
				assert(isMeasuring());
				m_total_time += getTime() - m_timer_start;
				m_total_count++;
				m_is_measuring = false;
			}
			std::string toString() const;

			/**
			 * \brief Used to sum statistics over several instances of this class.
			 */
			TimedStat& operator+=(const TimedStat &other) noexcept;
			/**
			 * \brief Used to average statistics over several instances of this class.
			 */
			TimedStat& operator/=(int i) noexcept;
	};

	/**
	 * \brief Class that starts timer in constructor and automatically handles stopping of timer in destructor.
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
