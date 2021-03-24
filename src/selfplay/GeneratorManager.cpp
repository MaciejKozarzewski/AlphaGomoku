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

	void GeneratorManager::generate(const std::string &pathToNetwork, int numberOfGames)
	{
		games_to_generate = numberOfGames;
		path_to_network = pathToNetwork;

		for (size_t i = 0; i < generators.size(); i++)
			thread_pool.addJob(generators[i].get());

		while (not thread_pool.isReady())
		{
			std::cout << game_buffer.size() << "/" << numberOfGames << '\n';
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	}

} /* namespace ag */

