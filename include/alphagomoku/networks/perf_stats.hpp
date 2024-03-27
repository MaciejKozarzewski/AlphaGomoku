/*
 * perf_stats.hpp
 *
 *  Created on: Mar 14, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_PERF_STATS_HPP_
#define ALPHAGOMOKU_NETWORKS_PERF_STATS_HPP_

#include <minml/core/Event.hpp>

#include <cstddef>

namespace ml
{
	class Context;
}

namespace ag
{
	struct PerfEvents
	{
			ml::Event start;
			ml::Event end;
			size_t batch_size;

			PerfEvents(int batchSize = 0) noexcept;
			bool isStarted() const noexcept;
			bool isFinished() const noexcept;
			double getTime() const noexcept;
	};

	class PerfEstimator
	{
			size_t currently_in_queue = 0;
			double estimated_end_time = 0.0;
			double count = 0.0;
			double average_time_per_sample = 0.0;
		public:
			void addToQueue(const PerfEvents &stats) noexcept;
			void updateTimePerSample(const PerfEvents &stats) noexcept;
			double getTimePerSample() const noexcept;
			double getEstimatedEndTime() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_PERF_STATS_HPP_ */
