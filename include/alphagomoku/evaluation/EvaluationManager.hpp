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
			EvaluationManager &manager;
			EvaluationQueue cross_queue;
			EvaluationQueue circle_queue;
			std::vector<std::unique_ptr<EvaluationGame>> evaluators;
			ml::Device device;
			int batch_size;
		public:
			EvaluatorThread(EvaluationManager &manager, const Json &options, ml::Device device);
			void setCrossPlayer(const Json &options, const std::string pathToNetwork);
			void setCirclePlayer(const Json &options, const std::string pathToNetwork);
			void run();
	};

	class EvaluationManager
	{
		private:
			ThreadPool thread_pool;
			std::vector<std::unique_ptr<EvaluatorThread>> evaluators;
			GameBuffer game_buffer;

			int games_to_play = 0;
		public:
			EvaluationManager(const Json &options);

			const GameBuffer& getGameBuffer() const noexcept;
			GameBuffer& getGameBuffer() noexcept;

			void setCrossPlayer(const Json &options, const std::string pathToNetwork);
			void setCirclePlayer(const Json &options, const std::string pathToNetwork);

			void generate(int numberOfGames);
			bool hasEnoughGames() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONMANAGER_HPP_ */
