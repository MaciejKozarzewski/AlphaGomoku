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
			pack("pack   "),
			launch("launch "),
			compute("compute"),
			wait("wait   "),
			unpack("unpack ")
	{
	}
	std::string NNEvaluatorStats::toString() const
	{
		std::string result = "----NNEvaluator----\n";
		result += "total samples = " + std::to_string(batch_sizes) + '\n';
		result += "avg batch size = " + std::to_string(static_cast<double>(batch_sizes) / compute.getTotalCount()) + '\n';
		result += pack.toString() + '\n';
		result += launch.toString() + '\n';
		result += compute.toString() + '\n';
		result += wait.toString() + '\n';
		result += unpack.toString() + '\n';
		return result;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator+=(const NNEvaluatorStats &other) noexcept
	{
		this->batch_sizes += other.batch_sizes;
		this->pack += other.pack;
		this->launch += other.launch;
		this->compute += other.compute;
		this->wait += other.wait;
		this->unpack += other.unpack;
		return *this;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator/=(int i) noexcept
	{
		this->batch_sizes /= i;
		this->pack /= i;
		this->launch /= i;
		this->compute /= i;
		this->wait /= i;
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
		network.moveTo(device);
		available_symmetries = 4 + 4 * static_cast<int>(is_square(network.getInputShape()));
	}
	void NNEvaluator::unloadGraph()
	{
		network.unloadGraph();
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
	SpeedSummary NNEvaluator::evaluateGraph()
	{
		if (not network.isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		SpeedSummary result;
		while (task_queue.size() > 0)
		{
			const int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
			if (batch_size > 0)
			{
				stats.batch_sizes += batch_size; // statistics

				pack_to_network(batch_size);

				stats.compute.startTimer();
				network.forward(batch_size);
				stats.compute.stopTimer();
				result.batch_size += batch_size;
				result.compute_time += stats.compute.getLastTime();

				unpack_from_network(batch_size);
				task_queue.erase(task_queue.begin(), task_queue.begin() + batch_size);
			}
		}
		return result;
	}
	void NNEvaluator::asyncEvaluateGraphLaunch()
	{
		if (not network.isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		if (task_queue.size() > (size_t) batch_size)
			throw std::logic_error("task queue size is larger than batch size");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		const int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
		if (batch_size > 0)
		{
			stats.batch_sizes += batch_size; // statistics

			pack_to_network(batch_size);

			stats.launch.startTimer();
			network.asyncForwardLaunch(batch_size);
			stats.launch.stopTimer();
			stats.compute.startTimer();
		}
	}
	SpeedSummary NNEvaluator::asyncEvaluateGraphJoin()
	{
		if (not network.isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		if (task_queue.size() > (size_t) batch_size)
			throw std::logic_error("task queue size is larger than batch size");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		const int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
		if (batch_size > 0)
		{
			stats.wait.startTimer();
			network.asyncForwardJoin();
			stats.wait.stopTimer();
			stats.compute.stopTimer();

			unpack_from_network(batch_size);
			task_queue.erase(task_queue.begin(), task_queue.begin() + batch_size);
		}
		return SpeedSummary( { batch_size, stats.compute.getLastTime(), stats.wait.getLastTime() });
	}
	/*
	 * private
	 */
	void NNEvaluator::pack_to_network(int batch_size)
	{
		TimerGuard timer(stats.pack);
		matrix<Sign> board(network.getInputShape()[1], network.getInputShape()[2]);
		for (int i = 0; i < batch_size; i++)
		{
			augment(board, task_queue[i].ptr->getBoard(), task_queue[i].symmetry);
			network.packInputData(i, board, task_queue[i].ptr->getSignToMove());
		}
	}
	void NNEvaluator::unpack_from_network(int batch_size)
	{
		TimerGuard timer(stats.unpack);
		matrix<float> policy(network.getInputShape()[1], network.getInputShape()[2]);
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

