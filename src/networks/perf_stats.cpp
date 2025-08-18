/*
 * perf_stats.cpp
 *
 *  Created on: Mar 14, 2024
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/perf_stats.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{

	/*
	 * PerfEvents
	 */
	PerfEvents::PerfEvents(int batchSize) noexcept :
			batch_size(batchSize)
	{
	}
	bool PerfEvents::isStarted() const noexcept
	{
		return start.isReady();
	}
	bool PerfEvents::isFinished() const noexcept
	{
		return end.isReady();
	}
	double PerfEvents::getTime() const noexcept
	{
		assert(isStarted());
		assert(isFinished());
		return ml::Event::getElapsedTime(start, end);
	}

	/*
	 * PerfEstimator
	 */
	void PerfEstimator::addToQueue(const PerfEvents &stats) noexcept
	{
		if (currently_in_queue == 0)
			estimated_end_time = getTime();
		estimated_end_time += getTimePerSample() * stats.batch_size;
		currently_in_queue += stats.batch_size;
	}
	void PerfEstimator::updateTimePerSample(const PerfEvents &stats) noexcept
	{
		assert(stats.batch_size != 0);
		count++;
		currently_in_queue -= stats.batch_size;
		average_time_per_sample += (stats.getTime() / stats.batch_size - average_time_per_sample) / count;
	}
	double PerfEstimator::getTimePerSample() const noexcept
	{
		return average_time_per_sample;
	}
	double PerfEstimator::getEstimatedEndTime() const noexcept
	{
		return estimated_end_time;
	}

} /* namespace ag */
