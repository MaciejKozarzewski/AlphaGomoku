/*
 * SupervisedLearning.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_
#define ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_

#include <alphagomoku/utils/configs.hpp>

#include <memory>
#include <string>
#include <vector>

class Json;
namespace ag
{
	class GameDataBuffer;
	class AGNetwork;
} /* namespace ag */

namespace ag
{
	class SupervisedLearning
	{
			int learning_steps = 0;
			std::vector<float> training_loss;
			std::vector<float> training_accuracy;
			std::vector<float> validation_loss;
			std::vector<float> validation_accuracy;

			TrainingConfig config;
		public:
			SupervisedLearning(const TrainingConfig &config);
			void updateTrainingStats(const std::vector<float> &loss, std::vector<float> &acc);
			void updateValidationStats(const std::vector<float> &loss, std::vector<float> &acc);
			void train(AGNetwork &model, GameDataBuffer &buffer, int steps);
			void validate(AGNetwork &model, GameDataBuffer &buffer);
			void saveTrainingHistory(const std::string &workingDirectory);
			void clearStats();

			Json saveProgress() const;
			void loadProgress(const Json &json);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_ */
