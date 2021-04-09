/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#include <alphagomoku/evaluation/EvaluationManager.hpp>

namespace ag
{

	EvaluatorThread::EvaluatorThread(const Json &options, ml::Device device) :
			evaluators(static_cast<int>(options["evaluation_options"]["games_per_thread"])),
			device(device),
			batch_size(evaluators.size() * static_cast<int>(options["evaluation_options"]["search_options"]["batch_size"]))
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluationGame>(GameConfig(options["game_options"]), game_buffer,
					options["evaluation_options"]["use_opening"]);
	}
	void EvaluatorThread::setFirstPlayer(const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		cross_queue.loadGraph(pathToNetwork, batch_size, device, static_cast<bool>(options["use_symmetries"]));
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, cross_queue, name);
	}
	void EvaluatorThread::setSecondPlayer(const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		circle_queue.loadGraph(pathToNetwork, batch_size, device, static_cast<bool>(options["use_symmetries"]));
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, circle_queue, name);
	}
	GameBuffer& EvaluatorThread::getGameBuffer() noexcept
	{
		return game_buffer;
	}
	void EvaluatorThread::generate(int numberOfGames)
	{
		games_to_play = numberOfGames;
	}
	void EvaluatorThread::run()
	{
		ml::Device::cpu().setNumberOfThreads(1);
		game_buffer.clear();
		while (game_buffer.size() < games_to_play)
		{
			for (size_t i = 0; i < evaluators.size() and game_buffer.size() < games_to_play; i++)
				evaluators[i]->generate();
			cross_queue.evaluateGraph();
			circle_queue.evaluateGraph();
		}
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->clear();
		cross_queue.unloadGraph();
		circle_queue.unloadGraph();
	}

	EvaluationManager::EvaluationManager(const Json &options) :
			thread_pool(options["evaluation_options"]["threads"].size()),
			evaluators(options["evaluation_options"]["threads"].size())
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluatorThread>(options, ml::Device::fromString(options["evaluation_options"]["threads"][i]["device"]));
	}

	GameBuffer& EvaluationManager::getGameBuffer(int threadIndex) noexcept
	{
		return evaluators[threadIndex]->getGameBuffer();
	}
	void EvaluationManager::setFirstPlayer(int threadIndex, const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		thread_pool.waitForFinish();
		evaluators[threadIndex]->setFirstPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setSecondPlayer(int threadIndex, const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		thread_pool.waitForFinish();
		evaluators[threadIndex]->setSecondPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setFirstPlayer(const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		thread_pool.waitForFinish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setSecondPlayer(const Json &options, const std::string pathToNetwork, const std::string &name)
	{
		thread_pool.waitForFinish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, pathToNetwork, name);
	}

	int EvaluationManager::numberOfThreads() const noexcept
	{
		return thread_pool.size();
	}
	int EvaluationManager::numberOfGames() const noexcept
	{
		int result = 0;
		for (size_t i = 0; i < evaluators.size(); i++)
			result += evaluators[i]->getGameBuffer().size();
		return result;
	}
	void EvaluationManager::generate(int numberOfGames)
	{
		for (size_t i = 0; i < evaluators.size(); i++)
		{
			evaluators[i]->generate(numberOfGames / evaluators.size());
			thread_pool.addJob(evaluators[i].get());
		}

		int time_counter = 0;
		int game_counter = numberOfGames / 4;
		while (not thread_pool.isReady())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			time_counter++;
			if (time_counter % 60 == 0 or this->numberOfGames() >= game_counter)
			{
				std::cout << this->numberOfGames() << " games played...\n";
				time_counter = 0;
				game_counter += numberOfGames / 4;
			}
		}
	}

}
/* namespace ag */

