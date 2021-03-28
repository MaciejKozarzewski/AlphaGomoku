/*
 * TrainingManager.hpp
 *
 *  Created on: Mar 24, 2021
 *      Author: maciek
 */

#ifndef SELFPLAY_TRAININGMANAGER_HPP_
#define SELFPLAY_TRAININGMANAGER_HPP_

#include <alphagomoku/evaluation/EvaluationManager.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>

namespace ag
{

	class TrainingManager
	{
		private:
			Json config;
			Json metadata;
			std::string working_dir;

			GeneratorManager generator;
			EvaluationManager evaluator;
			SupervisedLearning supervised_learning;

		public:
			TrainingManager(const std::string &workingDirectory);

			void runIterationRL();
			void runIterationSL(const std::string &buffer_src);
		private:
			void initFolderTree();
			void saveMetadata();
			bool loadMetadata();
			void initModel();
			void generateGames();

			void train();
			void validate();
			void evaluate();
			void splitBuffer(const GameBuffer &buffer, int training_games, int validation_games);
			void loadBuffer(GameBuffer &result, const std::string &path);

			void saveSelfPlayStats(int total_time);
			void saveGameBufferStats();
			void saveTrainingStats(const std::string &stats);
			void saveEvaluatorStats(int total_time);

			int get_last_checkpoint() const;
			int get_best_checkpoint() const;
			float get_learning_rate() const;
	};

} /* namespace ag */

#endif /* SELFPLAY_TRAININGMANAGER_HPP_ */
