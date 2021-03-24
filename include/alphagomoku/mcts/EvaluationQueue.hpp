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

			std::string toString() const;
			QueueStats& operator+=(const QueueStats &other) noexcept;
	};

	class EvaluationQueue
	{
		private:
			std::vector<EvaluationRequest*> request_queue;
			AGNetwork network;

			QueueStats stats;
			double idle_start = -1.0;

		public:
			void clearStats() noexcept;
			QueueStats getStats() const noexcept;
			int getQueueSize() const noexcept;
			void clearQueue() noexcept;
			void loadGraph(const std::string &path, int batchSize, ml::Device device = ml::Device::cpu());
			void unloadGraph();
			void addToQueue(EvaluationRequest &request);
			void evaluateGraph();
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EVALUATIONQUEUE_HPP_ */
