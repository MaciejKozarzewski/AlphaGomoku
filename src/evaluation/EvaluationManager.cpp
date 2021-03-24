/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#include <alphagomoku/evaluation/EvaluationManager.hpp>

namespace ag
{

	EvaluatorThread::EvaluatorThread(EvaluationManager &manager, const Json &options, ml::Device device) :
			manager(manager),
			evaluators(static_cast<int>(options["evaluation_options"]["games_per_thread"])),
			device(device),
			batch_size(evaluators.size() * static_cast<int>(options["evaluation_options"]["search_options"]["batch_size"]))
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluationGame>(GameConfig(options["game_options"]), manager.getGameBuffer(),
					options["evaluation_options"]["use_opening"]);
	}
	void EvaluatorThread::setCrossPlayer(const Json &options, const std::string pathToNetwork)
	{
		cross_queue.loadGraph(pathToNetwork, batch_size, device);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setCrossPlayer(options, cross_queue);
	}
	void EvaluatorThread::setCirclePlayer(const Json &options, const std::string pathToNetwork)
	{
		circle_queue.loadGraph(pathToNetwork, batch_size, device);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setCirclePlayer(options, circle_queue);
	}
	void EvaluatorThread::run()
	{
		ml::Device::cpu().setNumberOfThreads(1);
		while (not manager.hasEnoughGames())
		{
			for (size_t i = 0; i < evaluators.size(); i++)
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
			evaluators[i] = std::make_unique<EvaluatorThread>(*this, options,
					ml::Device::fromString(options["evaluation_options"]["threads"][i]["device"]));
	}

	const GameBuffer& EvaluationManager::getGameBuffer() const noexcept
	{
		return game_buffer;
	}
	GameBuffer& EvaluationManager::getGameBuffer() noexcept
	{
		return game_buffer;
	}
	void EvaluationManager::setCrossPlayer(const Json &options, const std::string pathToNetwork)
	{
		thread_pool.waitForFinish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setCrossPlayer(options, pathToNetwork);
	}
	void EvaluationManager::setCirclePlayer(const Json &options, const std::string pathToNetwork)
	{
		thread_pool.waitForFinish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setCirclePlayer(options, pathToNetwork);
	}
	bool EvaluationManager::hasEnoughGames() const noexcept
	{
		return game_buffer.size() >= games_to_play;
	}

	void EvaluationManager::generate(int numberOfGames)
	{
		games_to_play = numberOfGames;

		for (size_t i = 0; i < evaluators.size(); i++)
			thread_pool.addJob(evaluators[i].get());

		while (not thread_pool.isReady())
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			std::cout << getGameBuffer().getStats().toString() << '\n';
		}
	}

}
/* namespace ag */

