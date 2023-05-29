/*
 * TrainingManager.hpp
 *
 *  Created on: Mar 24, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef SELFPLAY_TRAININGMANAGER_HPP_
#define SELFPLAY_TRAININGMANAGER_HPP_

#include <alphagomoku/dataset/Dataset.hpp>
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
			Dataset training_dataset;
			Dataset validation_dataset;

			std::future<void> evaluation_future;
			std::mutex rating_mutex;
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

			void train_and_validate();
			void evaluate();
			void splitBuffer(GameDataBuffer &buffer, int training_games, int validation_games);
			void loadDataset(Dataset &result, const std::string &path);

			int get_last_checkpoint() const;
			void save_buffer_stats(const GameDataBuffer &buffer) const;
	};

} /* namespace ag */

#endif /* SELFPLAY_TRAININGMANAGER_HPP_ */
