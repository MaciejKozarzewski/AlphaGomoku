/*
 * NNEvaluator.cpp
 *
 *  Created on: Feb 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/configs.hpp>

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
	NNEvaluatorStats::NNEvaluatorStats() :
			pack("pack  "),
			eval_sync("eval  "),
			eval_async("eval_async"),
			unpack("unpack")
	{
	}
	std::string NNEvaluatorStats::toString() const
	{
		std::string result = "----NNEvaluator----\n";
		result += "avg batch size = " + std::to_string(static_cast<double>(batch_sizes) / eval_sync.getTotalCount()) + '\n';
		result += pack.toString() + '\n';
		result += eval_sync.toString() + '\n';
		result += eval_async.toString() + '\n';
		result += unpack.toString() + '\n';
		return result;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator+=(const NNEvaluatorStats &other) noexcept
	{
		this->batch_sizes += other.batch_sizes;
		this->pack += other.pack;
		this->eval_sync += other.eval_sync;
		this->eval_async += other.eval_async;
		this->unpack += other.unpack;
		return *this;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator/=(int i) noexcept
	{
		this->batch_sizes /= i;
		this->pack /= i;
		this->eval_sync /= i;
		this->eval_async /= i;
		this->unpack /= i;
		return *this;
	}

	NNEvaluator::NNEvaluator(const DeviceConfig &cfg) :
			device(cfg.device),
			batch_size(cfg.batch_size),
			omp_threads(cfg.omp_threads)
	{
	}
	bool NNEvaluator::isOnGPU() const noexcept
	{
		return device.isCUDA();
	}
	void NNEvaluator::clearStats() noexcept
	{
		stats = NNEvaluatorStats();
	}
	NNEvaluatorStats NNEvaluator::getStats() const noexcept
	{
		return stats;
	}
	int NNEvaluator::getQueueSize() const noexcept
	{
		return task_queue.size();
	}
	void NNEvaluator::clearQueue() noexcept
	{
		task_queue.clear();
	}
	void NNEvaluator::useSymmetries(bool b) noexcept
	{
		use_symmetries = b;
	}
	void NNEvaluator::loadGraph(const std::string &path)
	{
		network.loadFromFile(path);
		network.setBatchSize(batch_size);
		network.getGraph().moveTo(device);
		available_symmetries = 4 + 4 * static_cast<int>(is_square(network.getGraph().getInputShape()));
	}
	void NNEvaluator::unloadGraph()
	{
		network.getGraph().context().synchronize();
		network.getGraph().clear();
	}
	void NNEvaluator::addToQueue(SearchTask &task)
	{
		if (use_symmetries)
			task_queue.push_back( { &task, randInt(available_symmetries) });
		else
			task_queue.push_back( { &task, 0 });
	}
	void NNEvaluator::addToQueue(SearchTask &task, int symmetry)
	{
		assert(abs(symmetry) <= available_symmetries);
		task_queue.push_back( { &task, symmetry });
	}
	void NNEvaluator::evaluateGraph()
	{
		if (network.getGraph().numberOfLayers() == 0)
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		while (task_queue.size() > 0)
		{
			int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
			if (batch_size > 0)
			{
				stats.batch_sizes += batch_size; // statistics

				pack_to_network(batch_size);

				stats.eval_sync.startTimer();
				network.forward(batch_size);
				stats.eval_sync.stopTimer();

				unpack_from_network(batch_size);
				task_queue.erase(task_queue.begin(), task_queue.begin() + batch_size);
			}
		}
	}
	void NNEvaluator::asyncEvaluateGraphLaunch()
	{
		if (network.getGraph().numberOfLayers() == 0)
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		assert(task_queue.size() <= batch_size);
		int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
		if (batch_size > 0)
		{
			stats.batch_sizes += batch_size; // statistics

			pack_to_network(batch_size);

			stats.eval_sync.startTimer();
			stats.eval_async.startTimer();
			network.asyncForwardLaunch(batch_size);
			stats.eval_async.pauseTimer();
		}
	}
	void NNEvaluator::asyncEvaluateGraphJoin()
	{
		if (network.getGraph().numberOfLayers() == 0)
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		assert(task_queue.size() <= batch_size);
		int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
		if (batch_size > 0)
		{
			stats.eval_async.resumeTimer();
			network.asyncForwardJoin();
			stats.eval_async.stopTimer();
			stats.eval_sync.stopTimer();

			unpack_from_network(batch_size);
			task_queue.erase(task_queue.begin(), task_queue.begin() + batch_size);
		}
	}
	/*
	 * private
	 */
	void NNEvaluator::pack_to_network(int batch_size)
	{
		TimerGuard timer(stats.pack);
		matrix<Sign> board(network.getGraph().getInputShape()[1], network.getGraph().getInputShape()[2]);
		for (int i = 0; i < batch_size; i++)
		{
			augment(board, task_queue[i].ptr->getBoard(), task_queue[i].symmetry);
			network.packInputData(i, board, task_queue[i].ptr->getSignToMove());
		}
	}
	void NNEvaluator::unpack_from_network(int batch_size)
	{
		TimerGuard timer(stats.unpack);
		matrix<float> policy(network.getGraph().getInputShape()[1], network.getGraph().getInputShape()[2]);
		matrix<Value> action_values;
		Value value;
		for (int i = 0; i < batch_size; i++)
		{
			network.unpackOutput(i, policy, action_values, value);
			augment(task_queue[i].ptr->getPolicy(), policy, -task_queue[i].symmetry);
			// TODO add processing of action values
			task_queue[i].ptr->setValue(value);
			task_queue[i].ptr->markAsReadyNetwork();
		}
	}
}

