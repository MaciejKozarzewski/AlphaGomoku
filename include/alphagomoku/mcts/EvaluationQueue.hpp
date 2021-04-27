/*
 * EvaluationQueue.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_
#define ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_

#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <string>
#include <vector>
#include <utility>

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

			std::string toString() const;
			QueueStats& operator+=(const QueueStats &other) noexcept;
			QueueStats& operator/=(int i) noexcept;
	};

	class EvaluationQueue
	{
		private:
			std::vector<std::pair<EvaluationRequest*, int>> request_queue;
			AGNetwork network;

			QueueStats stats;
			bool use_symmetries = false;
			int available_symmetries = 1;
		public:

			void clearStats() noexcept;
			QueueStats getStats() const noexcept;
			int getQueueSize() const noexcept;
			void clearQueue() noexcept;
			void loadGraph(const std::string &path, int batchSize, ml::Device device = ml::Device::cpu(), bool useSymmetries = false);
			void unloadGraph();
			void addToQueue(EvaluationRequest &request);
			void addToQueue(EvaluationRequest &request, int symmetry);
			void evaluateGraph();
		private:
			void pack(int batch_size);
			void unpack(int batch_size);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_ */
