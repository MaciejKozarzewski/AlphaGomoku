/*
 * statistics.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef UTILS_STATISTICS_CPP_
#define UTILS_STATISTICS_CPP_

#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	TimedStat::TimedStat(const std::string &name) :
			m_name(name)
	{
	}
	std::string TimedStat::getName() const
	{
		return m_name;
	}
	double TimedStat::getTimerStart() const noexcept
	{
		return m_timer_start;
	}
	double TimedStat::getTotalTime() const noexcept
	{
		return m_total_time;
	}
	uint64_t TimedStat::getTotalCount() const noexcept
	{
		return m_total_count;
	}
	bool TimedStat::isMeasuring() const noexcept
	{
		return m_is_measuring;
	}
	bool TimedStat::isPaused() const noexcept
	{
		return m_is_paused;
	}

	void TimedStat::reset() noexcept
	{
		m_is_measuring = false;
		m_is_paused = false;
		m_total_time = 0.0;
		m_total_count = 0;
	}
	void TimedStat::startTimer() noexcept
	{
		assert(isMeasuring() == false);
		m_is_measuring = true;
		m_timer_start = getTime();
	}
	void TimedStat::pauseTimer() noexcept
	{
		assert(isMeasuring());
		m_total_time += getTime() - m_timer_start;
		m_is_paused = true;
	}
	void TimedStat::resumeTimer() noexcept
	{
		assert(isMeasuring() && isPaused());
		m_timer_start = getTime();
		m_is_paused = false;
	}
	void TimedStat::stopTimer() noexcept
	{
		assert(isMeasuring());
		m_total_time += getTime() - m_timer_start;
		m_total_count++;
		m_is_measuring = false;
	}
	std::string TimedStat::toString() const
	{
		double time = (getTotalCount() == 0) ? 0.0 : getTotalTime() / getTotalCount();
		char unit = ' ';
		if (time < 1.0e-3)
		{
			time *= 1.0e6;
			unit = 'u';
		}
		else
		{
			if (time < 1.0)
			{
				time *= 1.0e3;
				unit = 'm';
			}
		}
		return getName() + " = " + std::to_string(getTotalTime()) + "s : " + std::to_string(getTotalCount()) + " : " + std::to_string(time) + ' '
				+ unit + 's';
	}
	TimedStat& TimedStat::operator+=(const TimedStat &other) noexcept
	{
//		assert(this->isMeasuring() == false && this->isPaused() == false);
//		assert(other.isMeasuring() == false && other.isPaused() == false);
		this->m_total_time += other.m_total_time;
		this->m_total_count += other.m_total_count;
		return *this;
	}
	TimedStat& TimedStat::operator/=(int i) noexcept
	{
//		assert(this->isMeasuring() == false && this->isPaused() == false);
		this->m_total_time /= i;
		this->m_total_count /= i;
		return *this;
	}

} /* namespace ag */

#endif /* UTILS_STATISTICS_CPP_ */
