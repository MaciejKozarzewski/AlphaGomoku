/*
 * EvaluationManager.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_

#include <alphagomoku/selfplay/EvaluationGame.hpp>
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
			std::vector<Game> game_buffer;
			int games_to_play = 0;
		public:
			EvaluatorThread(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index);
			void setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void addToBuffer(const Game &game);
			const std::vector<Game>& getGameBuffer() noexcept;
			void generate(int numberOfGames);
			void stop();
			bool isFinished() const;
		private:
			void run();
	};

	class EvaluationManager
	{
		private:
			std::vector<std::unique_ptr<EvaluatorThread>> evaluators;
		public:
			EvaluationManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions);

			const std::vector<Game>& getGameBuffer(int threadIndex) const;
			std::string getPGN() const;

			void setFirstPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(int threadIndex, const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setFirstPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, const NetworkLoader &loader, const std::string &name);

			int numberOfThreads() const noexcept;
			int numberOfGames() const noexcept;
			void generate(int numberOfGames);
		private:
			void wait_for_finish() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_ */
