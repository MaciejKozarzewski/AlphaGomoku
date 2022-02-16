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
	class GameBuffer;
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
			void updateTrainingStats(float pl, float vl, std::vector<float> &acc);
			void updateValidationStats(float pl, float vl, std::vector<float> &acc);
			void train(AGNetwork &model, GameBuffer &buffer, int steps);
			void validate(AGNetwork &model, GameBuffer &buffer);
			void saveTrainingHistory(const std::string &workingDirectory);

			Json saveProgress() const;
			void loadProgress(const Json &json);
		private:
			matrix<float> prepare_policy_target(const SearchData &sample);
			Value prepare_value_target(const SearchData &sample);
			matrix<Value> prepare_action_values_target(const SearchData &sample);
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_ */
