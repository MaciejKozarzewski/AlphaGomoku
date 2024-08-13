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
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <chrono>

namespace ag
{
	NNEvaluatorStats::NNEvaluatorStats() :
			pack("pack   "),
			compute("compute"),
			unpack("unpack ")
	{
	}
	std::string NNEvaluatorStats::toString() const
	{
		std::string result = "----NNEvaluator----\n";
		result += "total samples = " + std::to_string(batch_sizes) + '\n';
		result += "avg batch size = " + std::to_string(static_cast<double>(batch_sizes) / compute.getTotalCount()) + '\n';
		result += pack.toString() + '\n';
		result += compute.toString() + '\n';
		result += unpack.toString() + '\n';
		return result;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator+=(const NNEvaluatorStats &other) noexcept
	{
		this->batch_sizes += other.batch_sizes;
		this->pack += other.pack;
		this->compute += other.compute;
		this->unpack += other.unpack;
		return *this;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator/=(int i) noexcept
	{
		this->batch_sizes /= i;
		this->pack /= i;
		this->compute /= i;
		this->unpack /= i;
		return *this;
	}

	/*
	 * NNEvaluator
	 */
	NNEvaluator::NNEvaluator(const DeviceConfig &cfg) :
			config(cfg)
	{
	}
	bool NNEvaluator::isOnGPU() const noexcept
	{
		return config.device.isCUDA() or config.device.isOPENCL();
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
		get_network().setBatchSize(config.batch_size);
		get_network().moveTo(config.device);
		get_network().convertToHalfFloats();
		get_network().forward(1);
	}
	void NNEvaluator::unloadGraph()
	{
		get_network().unloadGraph();
	}
	void NNEvaluator::addToQueue(SearchTask &task)
	{
		const int r = available_symmetries(get_network().getGameConfig().rows, get_network().getGameConfig().cols);
		if (use_symmetries)
			waiting_queue.push_back( { &task, randInt(r) });
		else
			waiting_queue.push_back( { &task, 0 });
	}
	void NNEvaluator::addToQueue(SearchTask &task, int symmetry)
	{
		assert(abs(symmetry) <= available_symmetries(get_network().getGameConfig().rows, get_network().getGameConfig().cols));
		waiting_queue.push_back( { &task, symmetry });
	}
	double NNEvaluator::evaluateGraph()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(config.omp_threads);
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
				PerfEvents ev(batch_size);
				perf_estimator.addToQueue(ev);

				ev.start = get_network().addEvent();
				get_network().forward(batch_size);
				ev.end = get_network().addEvent();
				stats.compute.stopTimer();

				unpack_from_network();
				in_progress_queue.clear();

				ev.end.synchronize();
				perf_estimator.updateTimePerSample(ev);
			}
		}
		return perf_estimator.getTimePerSample();
	}
	double NNEvaluator::asyncEvaluateGraphLaunch()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		if (not in_progress_queue.empty())
			throw std::logic_error("some tasks are already being processed");
		ml::Device::cpu().setNumberOfThreads(config.omp_threads);
		const int batch_size = std::min(static_cast<int>(waiting_queue.size()), get_network().getBatchSize());
		if (batch_size > 0)
		{
			in_progress_queue.assign(waiting_queue.begin(), waiting_queue.begin() + batch_size);
			waiting_queue.erase(waiting_queue.begin(), waiting_queue.begin() + batch_size);

			pack_to_network();

			perf_events.emplace_back(batch_size);
			perf_estimator.addToQueue(perf_events.back());

			perf_events.back().start = get_network().addEvent();
			get_network().asyncForwardLaunch(batch_size);
			perf_events.back().end = get_network().addEvent();

			stats.compute.startTimer();
		}
		return perf_estimator.getEstimatedEndTime();
	}
	void NNEvaluator::asyncEvaluateGraphJoin()
	{
		if (not get_network().isLoaded())
			throw std::logic_error("graph is empty - the network has not been loaded");
		ml::Device::cpu().setNumberOfThreads(config.omp_threads);
		const int batch_size = in_progress_queue.size();
		assert(batch_size <= get_network().getBatchSize());
		if (batch_size > 0)
		{
			get_network().asyncForwardJoin();
			stats.compute.stopTimer();
			stats.batch_sizes += batch_size; // statistics

			unpack_from_network();
			in_progress_queue.clear();

			perf_events.back().end.synchronize();
			perf_estimator.updateTimePerSample(perf_events.back());
			perf_events.pop_back();
		}
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
		matrix<Sign> board(get_network().getGameConfig().rows, get_network().getGameConfig().cols);
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
		matrix<float> policy(get_network().getGameConfig().rows, get_network().getGameConfig().cols);
		matrix<Value> action_values(get_network().getGameConfig().rows, get_network().getGameConfig().cols);
		Value value;
		for (size_t i = 0; i < in_progress_queue.size(); i++)
		{
			TaskData td = in_progress_queue.at(i);
			get_network().unpackOutput(i, policy, action_values, value);
			augment(td.ptr->getPolicy(), policy, -td.symmetry);
			augment(td.ptr->getActionValues(), action_values, -td.symmetry); // TODO silently assuming that action values come from TSS
			td.ptr->setValue(value);
			td.ptr->markAsProcessedByNetwork();
		}
	}
}

