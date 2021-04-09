/*
 * EvaluationManager.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_

#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/evaluation/EvaluationGame.hpp>
#include <alphagomoku/utils/ThreadPool.hpp>

#include <inttypes.h>
#include <string>

namespace ag
{
	class EvaluationManager;
} /* namespace ag */

namespace ag
{

	class EvaluatorThread: public Job
	{
		private:
			EvaluationQueue cross_queue;
			EvaluationQueue circle_queue;
			std::vector<std::unique_ptr<EvaluationGame>> evaluators;

			GameBuffer game_buffer;
			int games_to_play = 0;

			ml::Device device;
			int batch_size;
		public:
			EvaluatorThread(const Json &options, ml::Device device);
			void setFirstPlayer(const Json &options, const std::string pathToNetwork, const std::string &name);
			void setSecondPlayer(const Json &options, const std::string pathToNetwork, const std::string &name);
			GameBuffer& getGameBuffer() noexcept;
			void generate(int numberOfGames);
			void run();
	};

	class EvaluationManager
	{
		private:
			ThreadPool thread_pool;
			std::vector<std::unique_ptr<EvaluatorThread>> evaluators;
		public:
			EvaluationManager(const Json &options);

			GameBuffer& getGameBuffer(int threadIndex) noexcept;

			void setFirstPlayer(int threadIndex, const Json &options, const std::string pathToNetwork, const std::string &name);
			void setSecondPlayer(int threadIndex, const Json &options, const std::string pathToNetwork, const std::string &name);
			void setFirstPlayer(const Json &options, const std::string pathToNetwork, const std::string &name);
			void setSecondPlayer(const Json &options, const std::string pathToNetwork, const std::string &name);

			int numberOfThreads() const noexcept;
			int numberOfGames() const noexcept;
			void generate(int numberOfGames);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_ */
