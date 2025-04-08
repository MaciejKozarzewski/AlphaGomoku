/*
 * Model.hpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_
#define ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/networks/NetworkDataPack.hpp>
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

	class AGNetwork
	{
		protected:
			GameConfig game_config;
			ml::Graph graph;
			ml::Tensor input_on_device;

			NetworkDataPack data_pack;
		public:
			AGNetwork() noexcept;
			AGNetwork(const AGNetwork &other) = delete;
			AGNetwork(AGNetwork &&other) = delete;
			AGNetwork& operator=(const AGNetwork &other) = delete;
			AGNetwork& operator=(AGNetwork &&other) = delete;
			virtual ~AGNetwork() = default;

			virtual std::string getOutputConfig() const = 0;
			virtual std::string name() const = 0;

			std::vector<float> getAccuracy(int batchSize, int top_k = 4) const;
			void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
			/*
			 * \brief Can be used to pack the data if the features were already calculated.
			 */
			void packInputData(int index, const NNInputFeatures &features);
			void unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value, float &movesLeft) const;

			void asyncForwardLaunch(int batch_size, NetworkDataPack &pack);
			void asyncForwardLaunch(int batch_size);
			void asyncForwardJoin();
			void forward(int batch_size, NetworkDataPack &pack);
			void forward(int batch_size);
			std::vector<float> train(int batch_size, NetworkDataPack &pack);
			std::vector<float> train(int batch_size);
			std::vector<float> getLoss(int batch_size, const NetworkDataPack &pack);
			std::vector<float> getLoss(int batch_size);

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

			virtual int getBatchSize() const noexcept;
			virtual void setBatchSize(int batchSize);
			GameConfig getGameConfig() const noexcept;
			ml::Event addEvent() const;
			ml::Graph& get_graph()
			{
				return graph;
			}
			const NetworkDataPack& getDataPack() const noexcept
			{
				return data_pack;
			}
		protected:
			ml::Shape get_input_shape() const noexcept;
			void copy_input(const NetworkDataPack &pack);
			virtual void create_network(const TrainingConfig &trainingOptions) = 0;
	};

	std::unique_ptr<AGNetwork> createAGNetwork(const std::string &architecture);
	std::unique_ptr<AGNetwork> loadAGNetwork(const std::string &path);

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_AGNETWORK_HPP_ */
