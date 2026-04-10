/*
 * EvaluationThread.cpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/evaluation/EvaluationThread.hpp>
#include <alphagomoku/selfplay/NetworkLoader.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/tuning/GSPRT.hpp>

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
	void EvaluatorThread::setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		first_nn_evaluator.loadGraph(loader);
		first_nn_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, first_nn_evaluator, name);
	}
	void EvaluatorThread::setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		second_nn_evaluator.loadGraph(loader);
		second_nn_evaluator.useSymmetries(options.use_symmetries);
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, second_nn_evaluator, name);
	}
	void EvaluatorThread::addToBuffer(const TwoMatch &match)
	{
		std::lock_guard<std::mutex> lock(game_buffer_mutex);
		game_buffer.push_back(match);
	}
	const std::vector<TwoMatch>& EvaluatorThread::getGameBuffer() noexcept
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
	int EvaluatorThread::numberOfGames() const noexcept
	{
		return 2 * game_buffer.size();
	}
	/*
	 * private
	 */
	void EvaluatorThread::run()
	{
		game_buffer.clear();
		while (is_running.load() and numberOfGames() < games_to_play)
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

}
/* namespace ag */

