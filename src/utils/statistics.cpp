/*
 * statistics.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/statistics.hpp>

namespace ag
{
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
		this->m_total_time += other.m_total_time;
		this->m_total_count += other.m_total_count;
		return *this;
	}
	TimedStat& TimedStat::operator/=(int i) noexcept
	{
		this->m_total_time /= i;
		this->m_total_count /= i;
		return *this;
	}

} /* namespace ag */
