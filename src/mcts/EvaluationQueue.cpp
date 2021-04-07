/*
 * EvaluationQueue.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/Shape.hpp>
#include <libml/Tensor.hpp>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iostream>

namespace
{
	bool is_square(const ml::Shape &shape)
	{
		assert(shape.length() == 4);
		return shape[1] == shape[2];
	}
}

namespace ag
{
	std::string QueueStats::toString() const
	{
		std::string result = "----EvaluationQueue----\n";
		result += "avg batch size = " + std::to_string(static_cast<double>(batch_sizes) / nb_network_eval) + '\n';
		result += printStatistics("network  ", nb_network_eval, time_network_eval);
		result += printStatistics("pack     ", nb_pack, time_pack);
		result += printStatistics("unpack   ", nb_unpack, time_unpack);
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
		return *this;
	}
	QueueStats& QueueStats::operator/=(int i) noexcept
	{
		this->batch_sizes /= i;
		this->nb_network_eval /= i;
		this->nb_pack /= i;
		this->nb_unpack /= i;

		this->time_network_eval /= i;
		this->time_pack /= i;
		this->time_unpack /= i;
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
	void EvaluationQueue::loadGraph(const std::string &path, int batchSize, ml::Device device, bool useSymmetries)
	{
		network.loadFromFile(path);
		network.setBatchSize(batchSize);
		network.getGraph().moveTo(device);
		this->use_symmetries = useSymmetries;
		available_symmetries = 4 + 4 * static_cast<int>(is_square(network.getGraph().getInputShape()));
	}
	void EvaluationQueue::unloadGraph()
	{
		network.getGraph().context().synchronize();
		network.getGraph().clear();
	}
	void EvaluationQueue::addToQueue(EvaluationRequest &request)
	{
		request_queue.push_back( { &request, 0 });
	}
	void EvaluationQueue::evaluateGraph()
	{
		while (request_queue.size() > 0)
		{
			int batch_size = std::min(static_cast<int>(request_queue.size()), network.getBatchSize());
			if (batch_size > 0)
			{
				pack(batch_size);

				double start = getTime(); // statistics
				network.forward(batch_size);
				stats.time_network_eval += getTime() - start; // statistics
				stats.batch_sizes += batch_size; // statistics
				stats.nb_network_eval++; // statistics

				unpack(batch_size);
				request_queue.erase(request_queue.begin(), request_queue.begin() + batch_size);
			}
		}
	}
	// private
	void EvaluationQueue::pack(int batch_size)
	{
		double start = getTime(); // statistics
		matrix<Sign> board(network.getGraph().getInputShape()[1], network.getGraph().getInputShape()[2]);
		for (int i = 0; i < batch_size; i++)
		{
			if (use_symmetries)
			{
				request_queue[i].second = randInt(available_symmetries);
				augment(board, request_queue[i].first->getBoard(), request_queue[i].second);
			}
			network.packData(i, board, request_queue[i].first->getSignToMove());
		}
		stats.time_pack += getTime() - start; // statistics
		stats.nb_pack++; // statistics
	}
	void EvaluationQueue::unpack(int batch_size)
	{
		double start = getTime(); // statistics
		matrix<float> policy(network.getGraph().getInputShape()[1], network.getGraph().getInputShape()[2]);
		for (int i = 0; i < batch_size; i++)
		{
			if (use_symmetries)
			{
				Value value = network.unpackOutput(i, policy);
				augment(request_queue[i].first->getPolicy(), policy, -request_queue[i].second);
				request_queue[i].first->setValue(value);
			}
			else
			{
				Value value = network.unpackOutput(i, request_queue[i].first->getPolicy());
				request_queue[i].first->setValue(value);
			}
			request_queue[i].first->setReady();
		}
		stats.time_unpack += getTime() - start; // statistics
		stats.nb_unpack++; // statistics
	}
}

