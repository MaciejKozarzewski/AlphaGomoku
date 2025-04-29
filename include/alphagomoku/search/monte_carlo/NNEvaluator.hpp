/*
 * NNEvaluator.hpp
 *
 *  Created on: Feb 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_NNEVALUATOR_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_NNEVALUATOR_HPP_

#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/networks/perf_stats.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <string>
#include <vector>
#include <utility>

namespace ag
{
	class SearchTask;
	class DeviceConfig;
	class NetworkLoader;
}

namespace ag
{
	struct NNEvaluatorStats
	{
		public:
			uint64_t batch_sizes = 0;
			TimedStat pack;
			TimedStat compute;
			TimedStat unpack;

			NNEvaluatorStats();
			std::string toString() const;
			NNEvaluatorStats& operator+=(const NNEvaluatorStats &other) noexcept;
			NNEvaluatorStats& operator/=(int i) noexcept;
	};

	class NNEvaluator
	{
		private:
			struct TaskData
			{
					SearchTask *ptr = nullptr;
					int symmetry = 0;
			};

			std::vector<PerfEvents> perf_events;
			std::vector<TaskData> waiting_queue;
			std::vector<TaskData> in_progress_queue;
			std::unique_ptr<AGNetwork> network;

			PerfEstimator perf_estimator;
			NNEvaluatorStats stats;
			bool use_symmetries = false;
			DeviceConfig config;
		public:
			NNEvaluator(const DeviceConfig &cfg);

			bool isOnGPU() const noexcept;
			void clearStats() noexcept;
			NNEvaluatorStats getStats() const noexcept;
			bool isQueueFull() const noexcept;
			int getQueueSize() const noexcept;
			void clearQueue() noexcept;
			void useSymmetries(bool b) noexcept;

			void loadGraph(const NetworkLoader &loader);
			void unloadGraph();
			void addToQueue(SearchTask &task);
			void addToQueue(SearchTask &task, int symmetry);
			double evaluateGraph();
			double asyncEvaluateGraphLaunch();
			void asyncEvaluateGraphJoin();
		private:
			AGNetwork& get_network();
			const AGNetwork& get_network() const;
			void pack_to_network();
			void unpack_from_network();
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_NNEVALUATOR_HPP_ */
