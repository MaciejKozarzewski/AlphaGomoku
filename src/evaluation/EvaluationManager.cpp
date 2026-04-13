/*
 * EvaluationManager.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/evaluation/EvaluationManager.hpp>
#include <alphagomoku/selfplay/NetworkLoader.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/tuning/GSPRT.hpp>

#include <thread>

namespace ag
{

	EvaluationManager::EvaluationManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions) :
			evaluators(selfplayOptions.device_config.size())
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i] = std::make_unique<EvaluatorThread>(gameOptions, selfplayOptions, i);
	}

	const std::vector<TwoMatch>& EvaluationManager::getGameBuffer(int threadIndex) const
	{
		return evaluators.at(threadIndex)->getGameBuffer();
	}
	std::string EvaluationManager::getPGN() const
	{
		std::string result;
		for (size_t i = 0; i < evaluators.size(); i++)
		{
			const std::vector<TwoMatch> &buffer = getGameBuffer(i);
			for (size_t j = 0; j < buffer.size(); j++)
				result += buffer[j].getGame(0).generatePGN() + buffer[j].getGame(1).generatePGN();
		}
		return result;
	}
	void EvaluationManager::setFirstPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		wait_for_finish();
		evaluators.at(threadIndex)->setFirstPlayer(options, loader, name);
	}
	void EvaluationManager::setSecondPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		wait_for_finish();
		evaluators.at(threadIndex)->setSecondPlayer(options, loader, name);
	}
	void EvaluationManager::setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		wait_for_finish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setFirstPlayer(options, loader, name);
	}
	void EvaluationManager::setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name)
	{
		wait_for_finish();
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->setSecondPlayer(options, loader, name);
	}

	int EvaluationManager::numberOfThreads() const noexcept
	{
		return evaluators.size();
	}
	int EvaluationManager::numberOfGames() const noexcept
	{
		int result = 0;
		for (size_t i = 0; i < evaluators.size(); i++)
			result += evaluators[i]->numberOfGames();
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

			check_for_interruption();
			if (are_all_evaluators_done())
				break;
		}
	}
	void EvaluationManager::test(double eloDiff)
	{
		for (size_t i = 0; i < evaluators.size(); i++)
			evaluators[i]->generate(std::numeric_limits<int>::max());

		int time_counter = 0;
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			time_counter++;
			if (time_counter % 10 == 0)
			{
				std::vector<int> results;
				for (size_t i = 0; i < evaluators.size(); i++)
					results = join_vectors(results, convert_match_results(evaluators[i]->getGameBuffer()));

				GSPRT gsprt(eloDiff, 0.0, 0.05, 0.05);
				for (size_t i = 0; i < results.size(); i++)
				{
					gsprt.add_result(results[i]);
					if (gsprt.get_status() != -1)
					{
						std::cout << "GSPRT status = " << gsprt.get_status() << ", LLR = " << gsprt.get_LLR() << " after " << results.size() * 2
								<< " games\n";
						for (size_t i = 0; i < evaluators.size(); i++)
							evaluators[i]->stop();
						break;
					}
				}
				std::cout << "GSPRT LLR = " << gsprt.get_LLR() << '\n';
			}
			if (time_counter % 60 == 0)
			{
				std::cout << this->numberOfGames() << " games played..." << std::endl;
				time_counter = 0;
			}

			check_for_interruption();
			if (are_all_evaluators_done())
				break;
		}
	}
	/*
	 * private
	 */
	bool EvaluationManager::are_all_evaluators_done() const
	{
		bool result = true;
		for (size_t i = 0; i < evaluators.size(); i++)
			result &= evaluators[i]->isFinished();
		return result;
	}
	void EvaluationManager::wait_for_finish() const
	{
		while (not are_all_evaluators_done())
			std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	void EvaluationManager::check_for_interruption()
	{
		if (hasCapturedSignal(SignalType::INT))
		{
			std::cout << "Caught interruption signal" << std::endl;
			for (size_t i = 0; i < evaluators.size(); i++)
				evaluators[i]->stop();
			std::cout << "Evaluators stopped" << std::endl;
		}
	}

}
/* namespace ag */

