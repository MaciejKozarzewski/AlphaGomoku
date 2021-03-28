/*
 * GeneratorManager.cpp
 *
 *  Created on: Mar 22, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>

#include <stddef.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace ag
{
	GeneratorThread::GeneratorThread(GeneratorManager &manager, const Json &options, ml::Device device) :
			manager(manager),
			generators(static_cast<int>(options["selfplay_options"]["games_per_thread"])),
			device(device),
			batch_size(generators.size() * static_cast<int>(options["selfplay_options"]["search_options"]["batch_size"]))
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GameGenerator>(options, manager.getGameBuffer(), queue);
	}
	void GeneratorThread::run()
	{
		ml::Device::cpu().setNumberOfThreads(1);
		queue.loadGraph(manager.getPathToNetwork(), batch_size, device);
		while (not manager.hasEnoughGames())
		{
			for (size_t i = 0; i < generators.size(); i++)
				generators[i]->generate();
			queue.evaluateGraph();
		}
	}
	void GeneratorThread::resetGames()
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->reset();
	}
	void GeneratorThread::clearStats() noexcept
	{
		queue.clearStats();
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->clearStats();
	}
	QueueStats GeneratorThread::getQueueStats() const noexcept
	{
		return queue.getStats();
	}
	TreeStats GeneratorThread::getTreeStats() const noexcept
	{
		TreeStats result;
		for (size_t i = 0; i < generators.size(); i++)
			result += generators[i]->getTreeStats();
		result /= static_cast<int>(generators.size());
		return result;
	}
	CacheStats GeneratorThread::getCacheStats() const noexcept
	{
		CacheStats result;
		for (size_t i = 0; i < generators.size(); i++)
			result += generators[i]->getCacheStats();
		result /= static_cast<int>(generators.size());
		return result;
	}
	SearchStats GeneratorThread::getSearchStats() const noexcept
	{
		SearchStats result;
		for (size_t i = 0; i < generators.size(); i++)
			result += generators[i]->getSearchStats();
		result /= static_cast<int>(generators.size());
		return result;
	}

	GeneratorManager::GeneratorManager(const Json &options) :
			thread_pool(options["selfplay_options"]["threads"].size()),
			generators(options["selfplay_options"]["threads"].size())
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GeneratorThread>(*this, options,
					ml::Device::fromString(options["selfplay_options"]["threads"][i]["device"]));
	}

	const GameBuffer& GeneratorManager::getGameBuffer() const noexcept
	{
		return game_buffer;
	}
	GameBuffer& GeneratorManager::getGameBuffer() noexcept
	{
		return game_buffer;
	}
	std::string GeneratorManager::getPathToNetwork() const
	{
		return path_to_network;
	}
	bool GeneratorManager::hasEnoughGames() const noexcept
	{
		return game_buffer.size() >= games_to_generate;
	}
	void GeneratorManager::resetGames()
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->resetGames();
	}
	void GeneratorManager::generate(const std::string &pathToNetwork, int numberOfGames)
	{
		games_to_generate = numberOfGames;
		path_to_network = pathToNetwork;

		for (size_t i = 0; i < generators.size(); i++)
		{
			generators[i]->clearStats();
			thread_pool.addJob(generators[i].get());
		}

		int counter = 0;
		while (not thread_pool.isReady())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			counter++;
			if (counter % 60 == 0)
			{
				printStats();
				counter = 0;
			}
		}
	}
	void GeneratorManager::printStats()
	{
		std::cout << "Played games = " << game_buffer.size() << "/" << games_to_generate << '\n';
		std::cout << game_buffer.getStats().toString() << '\n';

		QueueStats queue_stats;
		SearchStats search_stats;
		TreeStats tree_stats;
		CacheStats cache_stats;
		for (size_t i = 0; i < generators.size(); i++)
		{
			queue_stats += generators[i]->getQueueStats();
			search_stats += generators[i]->getSearchStats();
			tree_stats += generators[i]->getTreeStats();
			cache_stats += generators[i]->getCacheStats();
		}
		queue_stats /= static_cast<int>(generators.size());
		search_stats /= static_cast<int>(generators.size());
		tree_stats /= static_cast<int>(generators.size());
		cache_stats /= static_cast<int>(generators.size());
		std::cout << queue_stats.toString();
		std::cout << search_stats.toString();
		std::cout << tree_stats.toString();
		std::cout << cache_stats.toString() << '\n';
	}

} /* namespace ag */

