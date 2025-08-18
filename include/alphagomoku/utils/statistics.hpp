/*
 * statistics.hpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_STATISTICS_HPP_
#define ALPHAGOMOKU_UTILS_STATISTICS_HPP_

#include <cinttypes>
#include <chrono>
#include <cassert>
#include <string>

namespace ag
{
	class TimedStat
	{
		private:
			using time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

			std::string m_name;
			time_point m_timer_start;
			time_point m_timer_stop;
			int64_t m_total_time = 0;
			int64_t m_total_count = 0;
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
			/*
			 * \brief Get total time in seconds.
			 */
			double getTotalTime() const noexcept
			{
				return m_total_time * 1.0e-9;
			}
			/*
			 * \brief Get time of the last measurement in seconds.
			 */
			double getLastTime() const noexcept
			{
				return std::chrono::duration<int64_t, std::nano>(m_timer_stop - m_timer_start).count() * 1.0e-9;
			}
			uint64_t getTotalCount() const noexcept
			{
				return m_total_count;
			}

			static time_point getTime() noexcept
			{
				return std::chrono::steady_clock::now();
			}

			/**
			 *\brief Reset all collected measurements.
			 */
			void reset() noexcept
			{
				m_total_time = 0;
				m_total_count = 0;
			}
			/**
			 * \brief Start time measurement.
			 */
			void startTimer() noexcept
			{
				m_timer_start = getTime();
				m_timer_stop = m_timer_start;
			}
			/**
			 * \brief Pause current time measurement.
			 * Similar to @see stopTimer() but it does not increase total counter.
			 */
			void pauseTimer() noexcept
			{
				m_timer_stop = getTime();
				m_total_time += std::chrono::duration<int64_t, std::nano>(m_timer_stop - m_timer_start).count();
			}
			/**
			 * \brief Resume the time measurement, paused by @see pauseTimer() call.
			 */
			void resumeTimer() noexcept
			{
				m_timer_start = getTime();
				m_timer_stop = m_timer_start;
			}
			/**
			 * \brief Stop current time measurement and increase the total counter.
			 */
			void stopTimer() noexcept
			{
				m_timer_stop = getTime();
				m_total_time += std::chrono::duration<int64_t, std::nano>(m_timer_stop - m_timer_start).count();
				m_total_count++;
			}
			/**
			 * \brief Stop current time measurement and increase the total counter by specified value
			 */
			void stopTimer(int count) noexcept
			{
//				assert(isMeasuring());
				m_timer_stop = getTime();
				m_total_time += std::chrono::duration<int64_t, std::nano>(m_timer_stop - m_timer_start).count();
				m_total_count += count;
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
