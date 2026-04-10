/*
 * EvaluationManager.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_

#include <alphagomoku/evaluation/EvaluationThread.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>

#include <cinttypes>
#include <string>
#include <future>

namespace ag
{
	class GameConfig;
	class SelfplayConfig;
	class NetworkLoader;
} /* namespace ag */

namespace ag
{

	class EvaluationManager
	{
		private:
			std::vector<std::unique_ptr<EvaluatorThread>> evaluators;
		public:
			EvaluationManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions);

			const std::vector<TwoMatch>& getGameBuffer(int threadIndex) const;
			std::string getPGN() const;

			void setFirstPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);

			int numberOfThreads() const noexcept;
			int numberOfGames() const noexcept;
			void generate(int numberOfGames);
			void test(double eloDiff);
		private:
			bool are_all_evaluators_done() const;
			void wait_for_finish() const;
			void check_for_interruption();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_ */
