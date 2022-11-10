/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <thread>

namespace ag
{

	EvaluatorThread::EvaluatorThread(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index) :
			first_nn_evaluator(selfplayOptions.device_config[index]),
			second_nn_evaluator(selfplayOptions.device_config[index]),
			evaluators(selfplayOptions.games_per_thread)
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluationGame>(gameOptions, game_buffer, selfplayOptions.use_opening, selfplayOptions.save_data);
	}
	void EvaluatorThread::setFirstPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		first_nn_evaluator.loadGraph(pathToNetwork);
		first_nn_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, first_nn_evaluator, name);
	}
	void EvaluatorThread::setSecondPlayer(const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		second_nn_evaluator.loadGraph(pathToNetwork);
		second_nn_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, second_nn_evaluator, name);
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
			return search_status == std::future_status::ready;
		}
		else
			return true;
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
			first_nn_evaluator.evaluateGraph();
			second_nn_evaluator.evaluateGraph();
		}
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->clear();
		first_nn_evaluator.unloadGraph();
		second_nn_evaluator.unloadGraph();
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
				std::cout << this->numberOfGames() << " games played..." << std::endl;
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

