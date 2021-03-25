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

namespace ag
{

	class TrainingManager
	{
		private:
			Json config;
			Json metadata;

			GeneratorManager generator;
			EvaluationManager evaluator;

		public:
			TrainingManager(const Json &options);

			int getNumberOfCheckpoints() const;
			void runIterationRL();
			void runIterationSL(const std::string &buffer_src);
		private:
			void initFolderTree();
			void saveMetadata();
			bool loadMetadata();
			void initModel(int blocks, int filters);
			void optimizeGraph(int id);
			void generateGames(int number);

			void train(int steps, const std::string &buffer_src);
			void validate(const std::string &buffer_src);
			void evaluate();
			void loadBuffer(GameBuffer &result, const std::string &path, int iterations);

			void saveSelfPlayStats(int total_time);
			void saveGameBufferStats();
			void saveTrainingStats(const std::string &stats);
			void saveEvaluatorStats(int total_time);
	};

} /* namespace ag */

#endif /* SELFPLAY_TRAININGMANAGER_HPP_ */
