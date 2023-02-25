/*
 * TrainingManager.hpp
 *
 *  Created on: Mar 24, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef SELFPLAY_TRAININGMANAGER_HPP_
#define SELFPLAY_TRAININGMANAGER_HPP_

#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>

#include <minml/utils/json.hpp>

namespace ag
{

	class TrainingManager
	{
		private:
			MasterLearningConfig config;
			Json metadata;
			std::string working_dir;
			std::string path_to_data;

			GeneratorManager generator_manager;
			EvaluationManager evaluator_manager;
			std::future<void> evaluation_future;
			std::mutex rating_mutex;
			SupervisedLearning supervised_learning_manager;

		public:
			TrainingManager(std::string workingDirectory, std::string pathToData = std::string());

			void runIterationRL();
			void runIterationSL();
		private:
			void initFolderTree();
			void saveMetadata();
			bool loadMetadata();
			void initModel();
			void generateGames();

			void train();
			void validate();
			void evaluate();
			void splitBuffer(GameDataBuffer &buffer, int training_games, int validation_games);
			void loadBuffer(GameDataBuffer &result, const std::string &path);

			int get_last_checkpoint() const;
	};

} /* namespace ag */

#endif /* SELFPLAY_TRAININGMANAGER_HPP_ */
