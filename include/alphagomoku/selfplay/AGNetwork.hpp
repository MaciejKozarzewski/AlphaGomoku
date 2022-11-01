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
#include <libml/graph/Graph.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>

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

			std::unique_ptr<ml::Tensor> input_on_cpu;

			std::unique_ptr<ml::Tensor> policy_on_cpu;
			std::unique_ptr<ml::Tensor> value_on_cpu;

			std::unique_ptr<ml::Tensor> policy_target;
			std::unique_ptr<ml::Tensor> value_target;

			int rows = 0;
			int cols = 0;

		public:
			AGNetwork() = default;
			AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions);

			std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;

			void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
			void packTargetData(int index, const matrix<float> &policy, matrix<Value> &actionValues, Value value);
			void unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const;

			void asyncForwardLaunch(int batch_size);
			void asyncForwardJoin();
			void forward(int batch_size);
			void backward(int batch_size);
			std::vector<ml::Scalar> getLoss(int batch_size);

			void changeLearningRate(float lr);
			ml::Graph& getGraph();

			/**
			 * This method makes the network non-trainable and then optimizes for inference.
			 */
			void optimize();

			void saveToFile(const std::string &path) const;
			void loadFromFile(const std::string &path);

			int getBatchSize() const noexcept;
			void setBatchSize(int batchSize);

		private:
			void create_network(const TrainingConfig &trainingOptions);
			void allocate_tensors();
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_AGNETWORK_HPP_ */
