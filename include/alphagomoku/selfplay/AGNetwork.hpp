/*
 * Model.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_
#define ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

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
	struct GameConfig;
	struct TrainingConfig;
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

			int rows = 0;
			int cols = 0;
		public:
			AGNetwork() = default;
			AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions);

			std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;

			void packInputData(int index, const matrix<uint32_t> &features);
			void packTargetData(int index, const matrix<float> &policy, matrix<Value> &actionValues, Value value);
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
			void create_network(const TrainingConfig &trainingOptions);
			void reallocate_tensors();
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_ */
