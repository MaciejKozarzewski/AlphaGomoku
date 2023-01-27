/*
 * NNEvaluator.hpp
 *
 *  Created on: Feb 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_NNEVALUATOR_HPP_
#define ALPHAGOMOKU_MCTS_NNEVALUATOR_HPP_

#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <string>
#include <vector>
#include <utility>

namespace ag
{
	class SearchTask;
	class DeviceConfig;
}

namespace ag
{
	struct NNEvaluatorStats
	{
		public:
			uint64_t batch_sizes = 0;
			TimedStat pack;
			TimedStat launch;
			TimedStat compute;
			TimedStat wait;
			TimedStat unpack;

			NNEvaluatorStats();
			std::string toString() const;
			NNEvaluatorStats& operator+=(const NNEvaluatorStats &other) noexcept;
			NNEvaluatorStats& operator/=(int i) noexcept;
	};

	struct SpeedSummary
	{
			int batch_size = 0;
			double compute_time = 0.0;
			double wait_time = 0.0;
	};

	class NNEvaluator
	{
		private:
			struct TaskData
			{
					SearchTask *ptr = nullptr;
					int symmetry = 0;
			};

			std::vector<TaskData> task_queue;
			AGNetwork network;

			NNEvaluatorStats stats;
			bool use_symmetries = false;
			int available_symmetries = 1;
			ml::Device device;
			int batch_size;
			int omp_threads;
		public:
			NNEvaluator(const DeviceConfig &cfg);

			bool isOnGPU() const noexcept;
			void clearStats() noexcept;
			NNEvaluatorStats getStats() const noexcept;
			int getQueueSize() const noexcept;
			void clearQueue() noexcept;
			void useSymmetries(bool b) noexcept;

			void loadGraph(const std::string &path);
			void unloadGraph();
			void addToQueue(SearchTask &task);
			void addToQueue(SearchTask &task, int symmetry);
			SpeedSummary evaluateGraph();
			void asyncEvaluateGraphLaunch();
			SpeedSummary asyncEvaluateGraphJoin();
		private:
			void pack_to_network(int batch_size);
			void unpack_from_network(int batch_size);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NNEVALUATOR_HPP_ */
