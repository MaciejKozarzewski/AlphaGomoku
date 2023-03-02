/*
 * Model.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_
#define ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/networks/NNInputFeatures.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <minml/graph/Graph.hpp>

#include <memory>
#include <vector>

class Json;
class SerializedObject;
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
	struct float3
	{
			float x, y, z;
	};

	class AGNetwork
	{
		protected:
			ml::Graph graph;
			ml::Context context_on_cpu;

			std::unique_ptr<ml::Tensor> input_on_cpu, input_on_device;
			std::vector<ml::Tensor> outputs_on_cpu;
			std::vector<ml::Tensor> targets_on_cpu;

			GameConfig game_config;
			const int POLICY_OUTPUT_INDEX = 0;
			const int VALUE_OUTPUT_INDEX = 1;
			const int ACTION_VALUES_OUTPUT_INDEX = 2;

			std::unique_ptr<PatternCalculator> pattern_calculator; // lazily allocated on first use
			NNInputFeatures input_features; // same as above
			mutable std::vector<float3> workspace;
		public:
			AGNetwork() noexcept;
			AGNetwork(const AGNetwork &other) = delete;
			AGNetwork(AGNetwork &&other) = delete;
			AGNetwork& operator=(const AGNetwork &other) = delete;
			AGNetwork& operator=(AGNetwork &&other) = delete;
			virtual ~AGNetwork() = default;

			virtual std::string name() const = 0;
			virtual std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;

			virtual void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
			/*
			 * \brief Can be used to pack the data if the features were already calculated.
			 */
			virtual void packInputData(int index, const NNInputFeatures &features);
			virtual void packTargetData(int index, const matrix<float> &policy, const matrix<Value> &actionValues, Value value);
			virtual void unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const;

			virtual void asyncForwardLaunch(int batch_size);
			virtual void asyncForwardJoin();
			virtual void forward(int batch_size);
			virtual void backward(int batch_size);
			virtual std::vector<float> getLoss(int batch_size);

			virtual void changeLearningRate(float lr);

			/**
			 * This method makes the network non-trainable and then optimizes for inference.
			 */
			virtual void optimize();
			virtual void convertToHalfFloats();

			virtual void init(const GameConfig &gameOptions, const TrainingConfig &trainingOptions);
			virtual void saveToFile(const std::string &path) const;
			virtual void loadFromFile(const std::string &path);
			virtual void loadFrom(const Json &json, const SerializedObject &so);
			virtual void unloadGraph();
			virtual bool isLoaded() const noexcept;

			virtual void synchronize();
			virtual void moveTo(ml::Device device);
			virtual ml::Shape getInputShape() const;

			virtual int getBatchSize() const noexcept;
			virtual void setBatchSize(int batchSize);
		protected:
			virtual void pack_input_to_graph(int batchSize);
			virtual PatternCalculator& get_pattern_calculator();
			virtual void create_network(const TrainingConfig &trainingOptions) = 0;
			virtual void reallocate_tensors();
	};

	std::unique_ptr<AGNetwork> createAGNetwork(const std::string &architecture);
	std::unique_ptr<AGNetwork> loadAGNetwork(const std::string &path);

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_ */
