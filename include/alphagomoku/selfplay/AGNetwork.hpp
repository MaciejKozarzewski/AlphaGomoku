/*
 * Model.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_
#define ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/selfplay/NNInputFeatures.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <minml/graph/Graph.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <memory>
#include <vector>

namespace ml
{
	class Shape;
	class Tensor;
} /* namespace ml */

namespace ag
{
	enum class GameOutcome;
	struct Value;
	class PatternCalculator;
} /* namespace ag */

namespace ag
{
	class AGNetwork
	{
		private:

			ml::Graph graph;
			ml::Context context_on_cpu;

			std::unique_ptr<ml::Tensor> input_on_cpu, input_on_device;
			std::unique_ptr<ml::Tensor> policy_on_cpu, action_values_on_cpu, value_on_cpu;
			std::unique_ptr<ml::Tensor> policy_target_on_cpu, action_values_target_on_cpu, value_target_on_cpu;

			GameConfig game_config;

			std::unique_ptr<PatternCalculator> pattern_calculator; // lazily allocated on first use
			NNInputFeatures input_features; // same as above
			mutable std::vector<float> workspace;
		public:
			AGNetwork() = default;
			AGNetwork(const GameConfig &gameOptions);
			AGNetwork(const GameConfig &gameOptions, const std::string &path);
			AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions);

			std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;

			void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
			/*
			 * \brief Can be used to pack the data if the features were already calculated.
			 */
			void packInputData(int index, const NNInputFeatures &features);
			void packTargetData(int index, const matrix<float> &policy, const matrix<Value> &actionValues, Value value);
			void unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const;

			void asyncForwardLaunch(int batch_size);
			void asyncForwardJoin();
			void forward(int batch_size);
			void backward(int batch_size);
			std::vector<float> getLoss(int batch_size);

			void changeLearningRate(float lr);

			/**
			 * This method makes the network non-trainable and then optimizes for inference.
			 */
			void optimize();
			void convertToHalfFloats();

			void saveToFile(const std::string &path) const;
			void loadFromFile(const std::string &path);
			void unloadGraph();
			bool isLoaded() const noexcept;

			void synchronize();
			void moveTo(ml::Device device);
			ml::Shape getInputShape() const;

			int getBatchSize() const noexcept;
			void setBatchSize(int batchSize);
		private:
			void pack_input_to_graph(int batchSize);
			PatternCalculator& getPatternCalculator();
			void create_network(const TrainingConfig &trainingOptions);
			void reallocate_tensors();
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_ */
