/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <thread>

namespace ag
{

	EvaluatorThread::EvaluatorThread(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index) :
			is_running(true),
			first_nn_evaluator(selfplayOptions.device_config[index]),
			second_nn_evaluator(selfplayOptions.device_config[index]),
			evaluators(selfplayOptions.games_per_thread)
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluationGame>(gameOptions, *this, selfplayOptions.use_opening);
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
	void EvaluatorThread::addToBuffer(const Game &game)
	{
		std::lock_guard<std::mutex> lock(game_buffer_mutex);
		game_buffer.push_back(game);
	}
	const std::vector<Game>& EvaluatorThread::getGameBuffer() noexcept
	{
		return game_buffer;
	}
	void EvaluatorThread::generate(int numberOfGames)
	{
		games_to_play = numberOfGames;
		evaluator_future = std::async(std::launch::async, [this]()
		{
			try
			{
				this->run();
			}
			catch (std::exception &e)
			{
				std::cout << "EvaluatorThread::generate() threw " << e.what() << std::endl;
				exit(-1);
			}
		});
	}
	void EvaluatorThread::stop()
	{
		is_running.store(false);
		evaluator_future.wait();
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
		while (is_running.load() and game_buffer.size() < (size_t) games_to_play)
		{
			first_nn_evaluator.asyncEvaluateGraphJoin();
			for (size_t i = 0; i < evaluators.size(); i++)
			{
				evaluators[i]->generate(1);
				if (first_nn_evaluator.isQueueFull())
				{
					first_nn_evaluator.asyncEvaluateGraphJoin();
					first_nn_evaluator.asyncEvaluateGraphLaunch();
				}
			}
			first_nn_evaluator.asyncEvaluateGraphJoin();
			first_nn_evaluator.asyncEvaluateGraphLaunch();

			second_nn_evaluator.asyncEvaluateGraphJoin();
			for (size_t i = 0; i < evaluators.size(); i++)
			{
				evaluators[i]->generate(2);
				if (second_nn_evaluator.isQueueFull())
				{
					second_nn_evaluator.asyncEvaluateGraphJoin();
					second_nn_evaluator.asyncEvaluateGraphLaunch();
				}
			}
			second_nn_evaluator.asyncEvaluateGraphJoin();
			second_nn_evaluator.asyncEvaluateGraphLaunch();
		}
		first_nn_evaluator.asyncEvaluateGraphJoin();
		second_nn_evaluator.asyncEvaluateGraphJoin();
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

	const std::vector<Game>& EvaluationManager::getGameBuffer(int threadIndex) const
	{
		return evaluators.at(threadIndex)->getGameBuffer();
	}
	std::string EvaluationManager::getPGN() const
	{
		std::string result;
		for (size_t i = 0; i < evaluators.size(); i++)
		{
			const std::vector<Game> &buffer = getGameBuffer(i);
			for (size_t j = 0; j < buffer.size(); j++)
				result += buffer[j].generatePGN();
		}
		return result;
	}
	void EvaluationManager::setFirstPlayer(int threadIndex, const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		evaluators.at(threadIndex)->setFirstPlayer(options, pathToNetwork, name);
	}
	void EvaluationManager::setSecondPlayer(int threadIndex, const SelfplayConfig &options, const std::string pathToNetwork, const std::string &name)
	{
		wait_for_finish();
		evaluators.at(threadIndex)->setSecondPlayer(options, pathToNetwork, name);
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
		const int progress_increment = numberOfGames / 4;
		int progress_counter = progress_increment;

		for (size_t i = 0; i < evaluators.size(); i++)
		{
			const int tmp = numberOfGames / (evaluators.size() - i);
			evaluators[i]->generate(tmp);
			numberOfGames -= tmp;
		}

		int time_counter = 0;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			time_counter++;
			if (time_counter % 60 == 0 or this->numberOfGames() >= progress_counter)
			{
				std::cout << this->numberOfGames() << " games played..." << std::endl;
				time_counter = 0;
				progress_counter += progress_increment;
			}

			if (hasCapturedSignal(SignalType::INT))
			{
				std::cout << "Caught interruption signal" << std::endl;
				for (size_t i = 0; i < evaluators.size(); i++)
					evaluators[i]->stop();
				std::cout << "Evaluators stopped" << std::endl;
				return;
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

