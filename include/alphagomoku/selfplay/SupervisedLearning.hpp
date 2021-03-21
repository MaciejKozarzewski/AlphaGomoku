/*
 * SupervisedLearning.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_
#define ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_

#include <libml/utils/json.hpp>
#include <memory>
#include <string>
#include <vector>

namespace ag
{
	class GameBuffer;
	class AGNetwork;
} /* namespace ag */

namespace ag
{
	void createFolder(const std::string &folder);

	class SupervisedLearning
	{
			int learning_steps = 0;
			std::vector<float> training_loss;
			std::vector<float> training_accuracy;
			std::vector<float> validation_loss;
			std::vector<float> validation_accuracy;

			Json config;
		public:

			SupervisedLearning(const Json &config);
			void updateTrainingStats(float pl, float vl, std::vector<float> &acc);
			void updateValidationStats(float pl, float vl, std::vector<float> &acc);
			void train(AGNetwork &model, GameBuffer &buffer, int steps);
			void validate(AGNetwork &model, GameBuffer &buffer);
			void saveTrainingHistory();

			Json saveProgress() const;
			void loadProgress(const Json &json);
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_SUPERVISEDLEARNING_HPP_ */
