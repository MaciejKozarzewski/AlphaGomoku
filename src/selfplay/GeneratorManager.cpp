/*
 * GeneratorManager.cpp
 *
 *  Created on: Mar 22, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <minml/core/Device.hpp>
#include <minml/utils/json.hpp>

#include <cstddef>
#include <cassert>
#include <thread>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <csignal>

namespace ag
{
	GeneratorThread::GeneratorThread(GeneratorManager &manager, const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index) :
			is_running(true),
			manager(manager),
			nn_evaluator(selfplayOptions.device_config[index]),
			generators(selfplayOptions.games_per_thread)
	{
		nn_evaluator.useSymmetries(selfplayOptions.use_symmetries);
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GameGenerator>(gameOptions, selfplayOptions, manager.getGameBuffer(), nn_evaluator);
	}
	void GeneratorThread::start()
	{
		generator_future = std::async(std::launch::async, [this]()
		{
			try
			{
				this->run();
			}
			catch(std::exception &e)
			{
				std::cout << "GeneratorThread::start() threw " << e.what() << '\n';
				exit(-1);
			}
		});
	}
	void GeneratorThread::stop()
	{
		is_running.store(false);
		generator_future.wait();
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
//		result /= static_cast<int>(generators.size());
		return result;
	}
	SearchStats GeneratorThread::getSearchStats() const noexcept
	{
		SearchStats result;
		for (size_t i = 0; i < generators.size(); i++)
			result += generators[i]->getSearchStats();
//		result /= static_cast<int>(generators.size());
		return result;
	}
	void GeneratorThread::saveGames(const std::string &path) const
	{
		if (not isFinished())
			throw std::logic_error("GeneratorThread::saveGames() : cannot save while the generator is running");

		Json json(JsonType::Array);
		SerializedObject so;
		for (size_t i = 0; i < generators.size(); i++)
			json[i] = generators[i]->save(so);

		FileSaver fs(path);
		fs.save(json, so, 2, true);
	}
	void GeneratorThread::loadGames(const std::string &path)
	{
		if (not isFinished())
			throw std::logic_error("GeneratorThread::loadGames() : cannot load while the generator is running");

		FileLoader fl(path, true);
		for (size_t i = 0; i < generators.size(); i++)
			generators[i]->load(fl.getJson()[i], fl.getBinaryData());
	}
	/*
	 * private
	 */
	void GeneratorThread::run()
	{
		nn_evaluator.loadGraph(manager.getPathToNetwork());
		while (is_running.load() and not manager.hasEnoughGames())
		{
			for (size_t i = 0; i < generators.size(); i++)
			{
				const GameGenerator::Status status = generators[i]->generate();
				if (nn_evaluator.isQueueFull() or status == GameGenerator::TASKS_NOT_READY)
				{
					nn_evaluator.asyncEvaluateGraphJoin();
					nn_evaluator.asyncEvaluateGraphLaunch();
				}
			}
		}
		nn_evaluator.asyncEvaluateGraphJoin();
		nn_evaluator.unloadGraph();
	}

	GeneratorManager::GeneratorManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions) :
			generators(selfplayOptions.device_config.size())
	{
		for (size_t i = 0; i < generators.size(); i++)
			generators[i] = std::make_unique<GeneratorThread>(*this, gameOptions, selfplayOptions, i);
	}

	void GeneratorManager::setWorkingDirectory(const std::string &path)
	{
		working_directory = path;
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
		load_state();

		for (size_t i = 0; i < generators.size(); i++)
		{
			generators[i]->clearStats();
			generators[i]->start();
		}

		int counter = 0;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			counter++;
			if (counter % 60 == 0)
				printStats();

			if (hasCapturedSignal(SignalType::INT))
			{
				std::cout << "Caught interruption signal" << std::endl;
				for (size_t i = 0; i < generators.size(); i++)
					generators[i]->stop();
				std::cout << "Generators stopped" << std::endl;
				save_state();
				return;
			}

			bool is_ready = true;
			for (size_t i = 0; i < generators.size(); i++)
				is_ready &= generators[i]->isFinished();
			if (is_ready)
				break;
		}
		if (counter < 60)
			printStats();
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
		std::cout << cache_stats.toString() << std::endl;
	}
	/*
	 * private
	 */
	void GeneratorManager::save_state()
	{
		if (working_directory.empty())
			return;

		std::string path = working_directory + "/saved_state/";
		if (not pathExists(path))
			createDirectory(path);

		std::cout << "Saving buffer" << std::endl;
		game_buffer.save(path + "buffer.bin");

		std::cout << "Saving games" << std::endl;
		for (size_t i = 0; i < generators.size(); i++)
		{
			const std::string tmp = path + "thread_" + std::to_string(i) + ".bin";
			generators[i]->saveGames(tmp);
			std::cout << "Saved " << tmp << std::endl;
		}
	}
	void GeneratorManager::load_state()
	{
		if (working_directory.empty())
			return;

		std::string path = working_directory + "/saved_state/";
		if (not pathExists(path))
		{
			std::cout << "No saved state was found" << std::endl;
			return;
		}

		if (pathExists(path + "buffer.bin"))
		{
			game_buffer.load(path + "buffer.bin");
			std::cout << "Loaded buffer:\n" << game_buffer.getStats().toString() << std::endl;
			removeFile(path + "buffer.bin");
		}

		for (size_t i = 0; i < generators.size(); i++)
		{
			const std::string tmp = path + "thread_" + std::to_string(i) + ".bin";
			if (pathExists(tmp))
			{
				generators[i]->loadGames(tmp);
				std::cout << "Loaded " << tmp << std::endl;
				removeFile(tmp);
			}
		}
	}

} /* namespace ag */

