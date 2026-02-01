/*
 * MovesLeftNetwork.hpp
 *
 *  Created on: Oct 9, 2025
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_MOVESLEFTNETWORK_HPP_
#define ALPHAGOMOKU_NETWORKS_MOVESLEFTNETWORK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/networks/NetworkDataPack.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
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
	struct Value;
	class PatternCalculator;
	class NNInputFeatures;
} /* namespace ag */

namespace ag
{
	class MovesLeftNetwork
	{
		protected:
			GameConfig game_config;
			ml::Graph graph;
			ml::Tensor input_on_cpu;
			ml::Tensor output_on_cpu;
			ml::Tensor target_on_cpu;
			PatternCalculator pattern_calculator; // lazily allocated on first use

		public:
			MovesLeftNetwork() noexcept;
			MovesLeftNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions) noexcept;

			std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;
			void packInputData(int index, const matrix<Sign> &board, Sign signToMove, GameRules rules);
			void packTarget(int index, int movesLeft);
			void unpackOutput(int index, float &movesLeft) const;

			void forward(int batch_size);
			std::vector<float> train(int batch_size);
			std::vector<float> getLoss(int batch_size);

			void changeLearningRate(float lr);

			void optimize(int level = 1);
			void convertToHalfFloats();

			void saveToFile(const std::string &path) const;
			void loadFromFile(const std::string &path);
			void loadFrom(const Json &json, const SerializedObject &so);
			void unloadGraph();
			bool isLoaded() const noexcept;

			void synchronize();
			void moveTo(ml::Device device);

			int getBatchSize() const noexcept;
			void setBatchSize(int batchSize);
			GameConfig getGameConfig() const noexcept;
			ml::Event addEvent() const;
			ml::Graph& get_graph()
			{
				return graph;
			}
		protected:
			ml::Shape get_input_shape() const noexcept;
			void allocate_tensors();
			void copy_input();
			void create_network(const TrainingConfig &trainingOptions);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_MOVESLEFTNETWORK_HPP_ */
