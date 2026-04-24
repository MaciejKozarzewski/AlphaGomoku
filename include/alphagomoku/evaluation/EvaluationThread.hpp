/*
 * EvaluationThread.hpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONTHREAD_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONTHREAD_HPP_

#include <alphagomoku/evaluation/EvaluationGame.hpp>
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

	class EvaluatorThread
	{
		private:
			std::future<void> evaluator_future;
			std::atomic<bool> is_running;
			NNEvaluator first_nn_evaluator;
			NNEvaluator second_nn_evaluator;
			std::vector<std::unique_ptr<EvaluationGame>> evaluators;

			std::mutex game_buffer_mutex;
			std::vector<TwoMatch> game_buffer;
			int games_to_play = 0;
		public:
			EvaluatorThread(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index);
			void setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);

			void addToBuffer(const TwoMatch &match);
			const std::vector<TwoMatch>& getGameBuffer() noexcept;
			void generate(int numberOfGames);
			void stop();
			bool isFinished() const;
			int numberOfGames() const noexcept;
		private:
			void run();
	};


} /* namespace ag */



#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONTHREAD_HPP_ */
