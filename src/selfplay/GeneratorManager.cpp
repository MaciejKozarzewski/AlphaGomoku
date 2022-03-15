/*
 * GeneratorManager.cpp
 *
 *  Created on: Mar 22, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>

#include <stddef.h>
#include <cassert>
#include <thread>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

namespace ag
{
	GeneratorThread::GeneratorThread(GeneratorManager &manager, const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index) :
			manager(manager),
			nn_evaluator(selfplayOptions.device_config[index]),
			generators(selfplayOptions.games_per_thread)
	{
		nn_evaluator.useSymmetries(selfplayOptions.use_symmetries);
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GameGenerator>(gameOptions, selfplayOptions, manager.getGameBuffer(), nn_evaluator);
	}
	void GeneratorThread::start(int epoch)
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->setEpoch(epoch);
		generator_future = std::async(std::launch::async, [this]()
		{	this->run();});
	}
	bool GeneratorThread::isFinished() const noexcept
	{
		if (generator_future.valid())
		{
			std::future_status search_status = generator_future.wait_for(std::chrono::milliseconds(0));
			return search_status == std::future_status::ready;
		}
		else
			return true;
	}
	void GeneratorThread::resetGames()
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->reset();
	}
	void GeneratorThread::clearStats() noexcept
	{
		nn_evaluator.clearStats();
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->clearStats();
	}
	NNEvaluatorStats GeneratorThread::getEvaluatorStats() const noexcept
	{
		return nn_evaluator.getStats();
	}
	NodeCacheStats GeneratorThread::getCacheStats() const noexcept
	{
		NodeCacheStats result;
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
	/*
	 * private
	 */
	void GeneratorThread::run()
	{
		nn_evaluator.loadGraph(manager.getPathToNetwork());
		while (not manager.hasEnoughGames())
		{
			for (size_t i = 0; i < generators.size(); i++)
				generators[i]->generate();
			nn_evaluator.evaluateGraph();
		}
		nn_evaluator.unloadGraph();
	}

	GeneratorManager::GeneratorManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions) :
			generators(selfplayOptions.device_config.size())
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GeneratorThread>(*this, gameOptions, selfplayOptions, i);
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
	void GeneratorManager::generate(const std::string &pathToNetwork, int numberOfGames, int epoch)
	{
		games_to_generate = numberOfGames;
		path_to_network = pathToNetwork;

		for (size_t i = 0; i < generators.size(); i++)
		{
			generators[i]->clearStats();
			generators[i]->start(epoch);
		}

		int counter = 0;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			counter++;
			if (counter % 60 == 0)
			{
				printStats();
				counter = 0;
			}
			bool is_ready = true;
			for (size_t i = 0; i < generators.size(); i++)
				is_ready &= generators[i]->isFinished();
			if (is_ready)
				break;
		}
	}
	void GeneratorManager::printStats()
	{
		std::cout << "Played games = " << game_buffer.size() << "/" << games_to_generate << '\n';
		std::cout << game_buffer.getStats().toString() << '\n';

		NNEvaluatorStats evaluator_stats;
		SearchStats search_stats;
		NodeCacheStats cache_stats;
		for (size_t i = 0; i < generators.size(); i++)
		{
			evaluator_stats += generators[i]->getEvaluatorStats();
			search_stats += generators[i]->getSearchStats();
			cache_stats += generators[i]->getCacheStats();
		}
		evaluator_stats /= static_cast<int>(generators.size());
		search_stats /= static_cast<int>(generators.size());
		cache_stats /= static_cast<int>(generators.size());
		std::cout << evaluator_stats.toString();
		std::cout << search_stats.toString();
		std::cout << cache_stats.toString() << '\n';
	}

} /* namespace ag */

