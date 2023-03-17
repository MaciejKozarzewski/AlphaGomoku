/*
 * NNEvaluator.cpp
 *
 *  Created on: Feb 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <chrono>

namespace
{
	bool is_square(const ml::Shape &shape)
	{
		assert(shape.rank() == 4);
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
	bool NNEvaluator::isQueueFull() const noexcept
	{
		return getQueueSize() >= get_network().getBatchSize();
	}
	int NNEvaluator::getQueueSize() const noexcept
	{
		return waiting_queue.size();
	}
	void NNEvaluator::clearQueue() noexcept
	{
		waiting_queue.clear();
	}
	void NNEvaluator::useSymmetries(bool b) noexcept
	{
		use_symmetries = b;
	}
	void NNEvaluator::loadGraph(const std::string &path)
	{
		network = loadAGNetwork(path);
		get_network().setBatchSize(batch_size);
		get_network().moveTo(device);
		get_network().convertToHalfFloats();
		available_symmetries = 4 + 4 * static_cast<int>(is_square(get_network().getInputShape()));
	}
	void NNEvaluator::unloadGraph()
	{
		get_network().unloadGraph();
	}
	void NNEvaluator::addToQueue(SearchTask &task)
	{
		if (use_symmetries)
			waiting_queue.push_back( { &task, randInt(available_symmetries) });
		else
			waiting_queue.push_back( { &task, 0 });
	}
	void NNEvaluator::addToQueue(SearchTask &task, int symmetry)
	{
		assert(abs(symmetry) <= available_symmetries);
		waiting_queue.push_back( { &task, symmetry });
	}
	SpeedSummary NNEvaluator::evaluateGraph()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		SpeedSummary result;
		while (waiting_queue.size() > 0)
		{
			const int batch_size = std::min(static_cast<int>(waiting_queue.size()), get_network().getBatchSize());
			if (batch_size > 0)
			{
				stats.batch_sizes += batch_size; // statistics

				in_progress_queue.assign(waiting_queue.begin(), waiting_queue.begin() + batch_size);
				waiting_queue.erase(waiting_queue.begin(), waiting_queue.begin() + batch_size);

				pack_to_network();

				stats.compute.startTimer();
				get_network().forward(batch_size);
				stats.compute.stopTimer();
				result.batch_size += batch_size;
				result.compute_time += stats.compute.getLastTime();

				unpack_from_network();
				in_progress_queue.clear();
			}
		}
		return result;
	}
	void NNEvaluator::asyncEvaluateGraphLaunch()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		if (not in_progress_queue.empty())
			throw std::logic_error("some tasks are already being processed");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		const int batch_size = std::min(static_cast<int>(waiting_queue.size()), get_network().getBatchSize());
		if (batch_size > 0)
		{
			in_progress_queue.assign(waiting_queue.begin(), waiting_queue.begin() + batch_size);
			waiting_queue.erase(waiting_queue.begin(), waiting_queue.begin() + batch_size);

			pack_to_network();

			stats.launch.startTimer();
			get_network().asyncForwardLaunch(batch_size);
			stats.launch.stopTimer();
			stats.compute.startTimer();
		}
	}
	SpeedSummary NNEvaluator::asyncEvaluateGraphJoin()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(omp_threads);
		const int batch_size = in_progress_queue.size();
		assert(batch_size <= get_network().getBatchSize());
		if (batch_size > 0)
		{
			stats.wait.startTimer();
			get_network().asyncForwardJoin();
			stats.wait.stopTimer();
			stats.compute.stopTimer();
			stats.batch_sizes += batch_size; // statistics

			unpack_from_network();
			in_progress_queue.clear();
		}
		return SpeedSummary( { batch_size, stats.compute.getLastTime(), stats.wait.getLastTime() });
	}
	/*
	 * private
	 */
	AGNetwork& NNEvaluator::get_network()
	{
		if (network == nullptr)
			throw std::logic_error("NNEvaluator::get_network() : network has not been initialized");
		return *network;
	}
	const AGNetwork& NNEvaluator::get_network() const
	{
		if (network == nullptr)
			throw std::logic_error("NNEvaluator::get_network() : network has not been initialized");
		return *network;
	}
	void NNEvaluator::pack_to_network()
	{
		TimerGuard timer(stats.pack);
		matrix<Sign> board(get_network().getInputShape()[1], get_network().getInputShape()[2]);
		for (size_t i = 0; i < in_progress_queue.size(); i++)
		{
			TaskData td = in_progress_queue.at(i);
			if (td.ptr->wasProcessedBySolver())
			{
				td.ptr->getFeatures().augment(td.symmetry);
				get_network().packInputData(i, td.ptr->getFeatures());
			}
			else
			{
				augment(board, td.ptr->getBoard(), td.symmetry);
				get_network().packInputData(i, board, td.ptr->getSignToMove());
			}
		}
	}
	void NNEvaluator::unpack_from_network()
	{
		TimerGuard timer(stats.unpack);
		matrix<float> policy(get_network().getInputShape()[1], get_network().getInputShape()[2]);
		matrix<Value> action_values(get_network().getInputShape()[1], get_network().getInputShape()[2]);
		Value value;
		for (size_t i = 0; i < in_progress_queue.size(); i++)
		{
			TaskData td = in_progress_queue.at(i);
			get_network().unpackOutput(i, policy, action_values, value);
			augment(td.ptr->getPolicy(), policy, -td.symmetry);
			augment(td.ptr->getActionValues(), action_values, -td.symmetry);
			td.ptr->setValue(value);
			td.ptr->markAsProcessedByNetwork();
		}
	}
}

