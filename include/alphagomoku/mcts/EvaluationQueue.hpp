/*
 * EvaluationQueue.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_
#define ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_

#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <libml/graph/Graph.hpp>
#include <string>
#include <vector>

namespace ag
{
	struct QueueStats
	{
		public:
			uint64_t batch_sizes = 0;
			uint64_t nb_network_eval = 0;
			uint64_t nb_pack = 0;
			uint64_t nb_unpack = 0;

			double time_network_eval = 0.0;
			double time_pack = 0.0;
			double time_unpack = 0.0;
			double time_idle = 0.0;

			void print() const;
			QueueStats& operator+=(const QueueStats &other);
	};

	class EvaluationQueue
	{
		private:
			std::vector<EvaluationRequest*> request_queue;
			ml::Graph graph;

			ml::Device device;
			std::unique_ptr<ml::Tensor> input_on_cpu;
			std::unique_ptr<ml::Tensor> input_on_device;
			std::unique_ptr<ml::Tensor> policy_on_cpu;
			std::unique_ptr<ml::Tensor> value_on_cpu;

			QueueStats stats;
			double idle_start = -1.0;

		public:
			EvaluationQueue(int board_size, int batch_size, ml::Device device);

			ml::Device getDevice() const noexcept;
			void clearStats() noexcept;
			QueueStats getStats() const noexcept;
			int getQueueSize() const noexcept;
			void clearQueue() noexcept;
			void loadGraph(const std::string &path);
			void unloadGraph();
			void addToQueue(EvaluationRequest *request);
			void evaluateGraph();
		private:
			void packToNetwork(int batch);
			void unpackFromNetwork(int batch);
	};
}

#endif /* ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_ */
