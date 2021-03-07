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
	void QueueStats::print() const
	{
		std::cout << "----NNQueueStats----" << std::endl;
		std::cout << "avg batch_size = " << (float) batch_sizes / nb_network_eval << std::endl;
		std::cout << "nb_network_eval = " << nb_network_eval << " (" << batch_sizes << ")" << std::endl;
		std::cout << "nb_pack = " << nb_pack << std::endl;
		std::cout << "nb_unpack = " << nb_unpack << std::endl;

		print_statistics("time_network_eval", nb_network_eval, time_network_eval);
		print_statistics("time_pack", nb_pack, time_pack);
		print_statistics("time_unpack", nb_unpack, time_unpack);
		std::cout << "time_idle = " << time_idle << std::endl;
	}
	QueueStats& QueueStats::operator+=(const QueueStats &other)
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

	EvaluationQueue::EvaluationQueue(int board_size, int batch_size, ml::Device device) :
			device(device)
	{
		ml::Shape input_shape( { batch_size, board_size, board_size });
		input_on_cpu = std::make_unique<ml::Tensor>(input_shape, "float32", ml::Device::cpu());
		input_on_device = std::make_unique<ml::Tensor>(input_shape, "float32", device);

		policy_on_cpu = std::make_unique<ml::Tensor>(ml::Shape( { batch_size, board_size * board_size }), "float32", ml::Device::cpu());
		value_on_cpu = std::make_unique<ml::Tensor>(ml::Shape( { batch_size, 1 }), "float32", ml::Device::cpu());
	}
	ml::Device EvaluationQueue::getDevice() const noexcept
	{
		return graph.device();
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
	void EvaluationQueue::loadGraph(const std::string &path)
	{
		graph.context().synchronize();
		FileLoader fl(path);

		graph = ml::Graph(fl.getJson(), fl.getBinaryData());
		graph.moveTo(device);
	}
	void EvaluationQueue::addToQueue(EvaluationRequest *request)
	{
		request_queue.push_back(request);
	}
	void EvaluationQueue::evaluateGraph()
	{
//		if (idle_start != -1)
//			stats.time_idle += getTime() - idle_start;

		int batch = std::min(input_on_cpu->firstDim(), static_cast<int>(request_queue.size()));
		if (batch > 0)
		{
			packToNetwork(batch);

//			double start = getTime(); //statistics
			graph.forward(batch);
			graph.context().synchronize();
//			stats.time_network_eval += getTime() - start; //statistics
			stats.batch_sizes += batch; //statistics
			stats.nb_network_eval++; //statistics

			unpackFromNetwork(batch);
			request_queue.clear();
		}

//		idle_start = getTime();
	}
	//private
	void EvaluationQueue::packToNetwork(int batch)
	{
//		double start = getTime(); //statistics
//
		for (int i = 0; i < batch; i++)
			encodeInputTensor(input_on_cpu->data<float>( { i, 0, 0, 0 }), request_queue[i]->getBoard(), request_queue[i]->getSignToMove());
		if (!device.isCPU())
			input_on_device->copyFromAsync(graph.context(), *input_on_cpu);

//		stats.time_pack += getTime() - start; //statistics
		stats.nb_pack++; //statistics
	}
	void EvaluationQueue::unpackFromNetwork(int batch)
	{
//		double start = getTime();
		policy_on_cpu->copyFromAsync(graph.context(), graph.getOutput(0));
		value_on_cpu->copyFromAsync(graph.context(), graph.getOutput(1));

		for (int i = 0; i < batch; i++)
		{
			request_queue[i]->setValue(1.0f - value_on_cpu->get<float>( { i, 0 }));
			std::memcpy(request_queue[i]->getPolicy().data(), policy_on_cpu->data<float>( { i, 0 }),
					sizeof(float) * request_queue[i]->getPolicy().size());
			request_queue[i]->setReady();
		}

//		stats.time_unpack += getTime() - start; //statistics
		stats.nb_unpack++; //statistics
	}
}

