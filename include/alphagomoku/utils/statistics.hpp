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
			int64_t m_total_time = 0;
			int64_t m_total_count = 0;
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
			double getTotalTime() const noexcept
			{
				return m_total_time * 1.0e-9;
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

			static time_point getTime() noexcept
			{
				return std::chrono::steady_clock::now();
			}

			/**
			 *\brief Reset all collected measurements.
			 */
			void reset() noexcept
			{
				m_is_measuring = false;
				m_is_paused = false;
				m_total_time = 0;
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
				m_total_time += std::chrono::duration<int64_t, std::nano>(getTime() - m_timer_start).count();
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
				m_total_time += std::chrono::duration<int64_t, std::nano>(getTime() - m_timer_start).count();
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
