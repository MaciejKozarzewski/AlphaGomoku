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
			eval("eval  "),
			unpack("unpack")
	{
	}
	std::string NNEvaluatorStats::toString() const
	{
		std::string result = "----NNEvaluator----\n";
		result += "avg batch size = " + std::to_string(static_cast<double>(batch_sizes) / eval.getTotalCount()) + '\n';
		result += pack.toString();
		result += eval.toString();
		result += unpack.toString();
		return result;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator+=(const NNEvaluatorStats &other) noexcept
	{
		this->batch_sizes += other.batch_sizes;
		this->pack += other.pack;
		this->eval += other.eval;
		this->unpack += other.unpack;
		return *this;
	}
	NNEvaluatorStats& NNEvaluatorStats::operator/=(int i) noexcept
	{
		this->batch_sizes /= i;
		this->pack /= i;
		this->eval /= i;
		this->unpack /= i;
		return *this;
	}

	NNEvaluator::NNEvaluator(ml::Device device, int batchSize) :
			device(device),
			batch_size(batchSize)
	{
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
		while (task_queue.size() > 0)
		{
			int batch_size = std::min(static_cast<int>(task_queue.size()), network.getBatchSize());
			if (batch_size > 0)
			{
				stats.batch_sizes += batch_size; // statistics

				pack_to_network(batch_size);

				stats.eval.startTimer();
				network.forward(batch_size);
				stats.eval.stopTimer();

				unpack_from_network(batch_size);
				task_queue.erase(task_queue.begin(), task_queue.begin() + batch_size);
			}
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
			if (use_symmetries)
			{
				augment(board, task_queue[i].ptr->getBoard(), task_queue[i].symmetry);
				network.packData(i, board, task_queue[i].ptr->getSignToMove());
			}
			else
				network.packData(i, task_queue[i].ptr->getBoard(), task_queue[i].ptr->getSignToMove());
		}
	}
	void NNEvaluator::unpack_from_network(int batch_size)
	{
		TimerGuard timer(stats.unpack);

		matrix<float> policy(network.getGraph().getInputShape()[1], network.getGraph().getInputShape()[2]);
		for (int i = 0; i < batch_size; i++)
		{
			if (use_symmetries)
			{
				Value value = network.unpackOutput(i, policy);
				augment(task_queue[i].ptr->getPolicy(), policy, -task_queue[i].symmetry);
				task_queue[i].ptr->setValue(value);
			}
			else
			{
				Value value = network.unpackOutput(i, task_queue[i].ptr->getPolicy());
				task_queue[i].ptr->setValue(value);
			}
			task_queue[i].ptr->setReady();
		}
	}
}
