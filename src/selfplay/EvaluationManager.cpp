/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/EvaluationManager.hpp>

namespace ag
{

	EvaluatorThread::EvaluatorThread(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index) :
			cross_evaluator(selfplayOptions.device_config[index]),
			circle_evaluator(selfplayOptions.device_config[index]),
			evaluators(selfplayOptions.games_per_thread)
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluationGame>(gameOptions, game_buffer, selfplayOptions.use_opening);
	}
	void EvaluatorThread::setFirstPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		cross_evaluator.loadGraph(pathToNetwork);
		cross_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, cross_evaluator, name);
	}
	void EvaluatorThread::setSecondPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		circle_evaluator.loadGraph(pathToNetwork);
		circle_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, circle_evaluator, name);
	}
	GameBuffer& EvaluatorThread::getGameBuffer() noexcept
	{
		return game_buffer;
	}
	void EvaluatorThread::generate(int numberOfGames)
	{
		games_to_play = numberOfGames;
		evaluator_future = std::async(std::launch::async, [this]()
		{	this->run();});
	}
	bool EvaluatorThread::isFinished() const
	{
		if (evaluator_future.valid())
		{
			std::future_status search_status = evaluator_future.wait_for(std::chrono::milliseconds(0));
			return search_status != std::future_status::ready;
		}
		else
			return false;
	}
	/*
	 * private
	 */
	void EvaluatorThread::run()
	{
		game_buffer.clear();
		while (game_buffer.size() < games_to_play)
		{
			for (size_t i = 0; i < evaluators.size() and game_buffer.size() < games_to_play; i++)
				evaluators[i]->generate();
			cross_evaluator.evaluateGraph();
			circle_evaluator.evaluateGraph();
		}
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->clear();
		cross_evaluator.unloadGraph();
		circle_evaluator.unloadGraph();
	}

	EvaluationManager::EvaluationManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions) :
			evaluators(selfplayOptions.device_config.size())
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluatorThread>(gameOptions, selfplayOptions, i);
	}

	GameBuffer& EvaluationManager::getGameBuffer(int threadIndex) noexcept
	{
		return evaluators[threadIndex]->getGameBuffer();
	}
	void EvaluationManager::setFirstPlayer(int threadIndex, const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		evaluators[threadIndex]->setFirstPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setSecondPlayer(int threadIndex, const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		evaluators[threadIndex]->setSecondPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setFirstPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setSecondPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, pathToNetwork, name);
	}

	int EvaluationManager::numberOfThreads() const noexcept
	{
		return evaluators.size();
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
			evaluators[i]->generate(numberOfGames / evaluators.size());

		int time_counter = 0;
		int game_counter = numberOfGames / 4;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			time_counter++;
			if (time_counter % 60 == 0 or this->numberOfGames() >= game_counter)
			{
				std::cout << this->numberOfGames() << " games played...\n";
				time_counter = 0;
				game_counter += numberOfGames / 4;
			}

			bool is_ready = true;
			for (size_t i = 0; i < evaluators.size(); i++)
				is_ready &= evaluators[i]->isFinished();
			if (is_ready)
				break;
		}
	}
	/*
	 * private
	 */
	void EvaluationManager::wait_for_finish() const
	{
		while (true)
		{
			bool is_ready = true;
			for (size_t i = 0; i < evaluators.size(); i++)
				is_ready &= evaluators[i]->isFinished();
			if (is_ready)
				break;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

}
/* namespace ag */

