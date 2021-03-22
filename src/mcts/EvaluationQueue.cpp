/*
 * EvaluationQueue.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/Shape.hpp>
#include <libml/Tensor.hpp>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iostream>

namespace ag
{
	std::string QueueStats::toString() const
	{
		std::string result;
		result += "----EvaluationQueue----\n";
		result += "avg batch_size = " + std::to_string(static_cast<double>(batch_sizes) / nb_network_eval) + '\n';
		result += printStatistics("network_eval", nb_network_eval, time_network_eval);
		result += printStatistics("pack", nb_pack, time_pack);
		result += printStatistics("unpack", nb_unpack, time_unpack);
		result += "time_idle = " + std::to_string(time_idle) + "s\n";
		return result;
	}
	QueueStats& QueueStats::operator+=(const QueueStats &other) noexcept
	{
		this->batch_sizes += other.batch_sizes;
		this->nb_network_eval += other.nb_network_eval;
		this->nb_pack += other.nb_pack;
		this->nb_unpack += other.nb_unpack;

		this->time_network_eval += other.time_network_eval;
		this->time_pack += other.time_pack;
		this->time_unpack += other.time_unpack;
		this->time_idle += other.time_idle;
		return *this;
	}

	void EvaluationQueue::clearStats() noexcept
	{
		stats = QueueStats();
	}
	QueueStats EvaluationQueue::getStats() const noexcept
	{
		return stats;
	}
	int EvaluationQueue::getQueueSize() const noexcept
	{
		return request_queue.size();
	}
	void EvaluationQueue::clearQueue() noexcept
	{
		request_queue.clear();
	}
	void EvaluationQueue::loadGraph(const std::string &path, int batchSize, ml::Device device)
	{
		network.loadFromFile(path);
		network.setBatchSize(batchSize);
		network.getGraph().moveTo(device);
	}
	void EvaluationQueue::addToQueue(EvaluationRequest &request)
	{
		request_queue.push_back(&request);
	}
	void EvaluationQueue::evaluateGraph()
	{
		if (idle_start != -1)
			stats.time_idle += getTime() - idle_start; // statistics

		double start = getTime(); // statistics
		int batch = std::min(network.getBatchSize(), static_cast<int>(request_queue.size()));
		if (batch > 0)
		{
			for (int i = 0; i < batch; i++)
				network.packData(i, request_queue[i]->getBoard(), request_queue[i]->getSignToMove());
			stats.time_pack += getTime() - start; // statistics
			stats.nb_pack++;

			start = getTime();
			network.forward(batch);
			stats.time_network_eval += getTime() - start; // statistics
			stats.batch_sizes += batch; // statistics
			stats.nb_network_eval++; // statistics

			start = getTime();
			for (int i = 0; i < batch; i++)
			{
				float value = network.unpackOutput(i, request_queue[i]->getPolicy());
				request_queue[i]->setValue(1.0f - value);
				request_queue[i]->setReady();
			}
			request_queue.erase(request_queue.begin(), request_queue.begin() + batch);
			stats.time_unpack += getTime() - start; // statistics
			stats.nb_unpack++;
		}
		idle_start = getTime(); // statistics
	}
}

