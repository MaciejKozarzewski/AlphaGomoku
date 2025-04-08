/*
 * AGNetwork.cpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/networks/NNInputFeatures.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <alphagomoku/networks/networks.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/core/Event.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/Tensor.hpp>
#include <minml/core/math.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <bitset>

namespace ag
{

	AGNetwork::AGNetwork() noexcept
	{
	}
	std::vector<float> AGNetwork::getAccuracy(int batchSize, int top_k) const
	{
		return ag::getAccuracy(batchSize, data_pack, top_k);
	}
	void AGNetwork::packInputData(int index, const matrix<Sign> &board, Sign signToMove)
	{
		data_pack.packInputData(index, board, signToMove);
	}
	void AGNetwork::packInputData(int index, const NNInputFeatures &features)
	{
		data_pack.packInputData(index, features);
	}
	void AGNetwork::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value, float &movesLeft) const
	{
		data_pack.unpackPolicy(index, policy);
		data_pack.unpackValue(index, value);
		data_pack.unpackMovesLeft(index, movesLeft);
		data_pack.unpackActionValues(index, actionValues);
	}

	void AGNetwork::asyncForwardLaunch(int batch_size, NetworkDataPack &pack)
	{
		copy_input(pack);
		graph.predict(batch_size);
		const std::string output_config = getOutputConfig();
		assert((size_t ) graph.numberOfOutputs() == output_config.size());
		for (size_t i = 0; i < output_config.size(); i++)
			pack.getOutput(output_config[i]).copyFrom(graph.context(), graph.getOutput(i));
	}
	void AGNetwork::asyncForwardLaunch(int batch_size)
	{
		asyncForwardLaunch(batch_size, data_pack);
	}
	void AGNetwork::asyncForwardJoin()
	{
		graph.context().synchronize();
	}

	void AGNetwork::forward(int batch_size, NetworkDataPack &pack)
	{
		asyncForwardLaunch(batch_size, pack);
		graph.context().synchronize();
	}
	void AGNetwork::forward(int batch_size)
	{
		forward(batch_size, data_pack);
	}
	std::vector<float> AGNetwork::train(int batch_size, NetworkDataPack &pack)
	{
		assert(graph.isTrainable());
		copy_input(pack);

		const std::string output_config = getOutputConfig();
		assert((size_t ) graph.numberOfOutputs() == output_config.size());
		for (size_t i = 0; i < output_config.size(); i++)
		{
			graph.getTarget(i).copyFrom(graph.context(), pack.getTarget(output_config[i]));
			graph.getMask(i).copyFrom(graph.context(), pack.getMask(output_config[i]));
		}

		const bool success = graph.train(batch_size);
		for (size_t i = 0; i < output_config.size(); i++)
			pack.getOutput(output_config[i]).copyFrom(graph.context(), graph.getOutput(i));
		graph.context().synchronize();
		if (success)
			return graph.getLoss(batch_size);
		else
			return std::vector<float>();
	}
	std::vector<float> AGNetwork::train(int batch_size)
	{
		return train(batch_size, data_pack);
	}
	std::vector<float> AGNetwork::getLoss(int batch_size, const NetworkDataPack &pack)
	{
		assert(graph.isTrainable());
		const std::string output_config = getOutputConfig();
		assert((size_t ) graph.numberOfOutputs() == output_config.size());
		for (size_t i = 0; i < output_config.size(); i++)
			graph.getTarget(i).copyFrom(graph.context(), pack.getTarget(output_config[i]));
		return graph.getLoss(batch_size);
	}
	std::vector<float> AGNetwork::getLoss(int batch_size)
	{
		return getLoss(batch_size, data_pack);
	}

	void AGNetwork::changeLearningRate(float lr)
	{
		assert(graph.isTrainable());
		graph.getOptimizer().setLearningRate(lr);
	}
	void AGNetwork::optimize()
	{
		graph.makeTrainable(false);
		ml::FoldBatchNorm().optimize(graph);
		ml::FoldAdd().optimize(graph);
//		ml::FuseConvBlock().optimize(graph);
//		ml::FuseSEBlock().optimize(graph);
	}
	void AGNetwork::convertToHalfFloats()
	{
		ml::DataType new_type = ml::DataType::FLOAT32;
		if (graph.device().supportsType(ml::DataType::FLOAT16))
			new_type = ml::DataType::FLOAT16;
		if (new_type != ml::DataType::FLOAT32)
		{
			graph.convertTo(new_type);
			data_pack = NetworkDataPack(game_config, getBatchSize(), graph.dtype());
			if (not graph.device().isCPU())
				input_on_device = ml::Tensor(get_input_shape(), ml::DataType::INT32, graph.device());
		}
	}
	void AGNetwork::init(const GameConfig &gameOptions, const TrainingConfig &trainingOptions)
	{
		game_config = gameOptions;
		create_network(trainingOptions);
		data_pack = NetworkDataPack(game_config, getBatchSize(), graph.dtype());
		if (not graph.device().isCPU())
			input_on_device = ml::Tensor(get_input_shape(), ml::DataType::INT32, graph.device());
	}
	void AGNetwork::saveToFile(const std::string &path) const
	{
		graph.context().synchronize();
		SerializedObject so;
		Json json;
		json["architecture"] = name();
		json["config"] = game_config.toJson();
		json["model"] = graph.save(so);
		FileSaver fs(path);
		fs.save(json, so, 2);
	}
	void AGNetwork::loadFromFile(const std::string &path)
	{
		FileLoader fl(path);
		loadFrom(fl.getJson(), fl.getBinaryData());
	}
	void AGNetwork::loadFrom(const Json &json, const SerializedObject &so)
	{
		graph.context().synchronize();
		if (json["architecture"].getString() != name())
			throw std::runtime_error(
					"AGNetwork::loadFromFile() : saved model has architecture '" + json["architecture"].getString()
							+ "' and cannot be loaded into model '" + name() + "'");
		game_config = GameConfig(json["config"]);
		graph.load(json["model"], so);
	}
	void AGNetwork::unloadGraph()
	{
		synchronize();
		graph.clear();
	}
	bool AGNetwork::isLoaded() const noexcept
	{
		return graph.numberOfNodes() > 0;
	}
	void AGNetwork::synchronize()
	{
		graph.context().synchronize();
	}
	void AGNetwork::moveTo(ml::Device device)
	{
		graph.moveTo(device);
		if (graph.device().isCUDA())
			data_pack.pinMemory();
		if (not graph.device().isCPU())
			input_on_device = ml::Tensor(get_input_shape(), ml::DataType::INT32, graph.device());
	}

	int AGNetwork::getBatchSize() const noexcept
	{
		return graph.getInputShape().firstDim();
	}
	void AGNetwork::setBatchSize(int batchSize)
	{
		if (batchSize != graph.getInputShape().firstDim())
		{ // get here only if the batch size was actually changed
			ml::Shape shape = graph.getInputShape();
			shape[0] = batchSize;
			graph.setInputShape(shape);
			data_pack = NetworkDataPack(game_config, batchSize, graph.dtype());
			if (not graph.device().isCPU())
				input_on_device = ml::Tensor(get_input_shape(), ml::DataType::INT32, graph.device());
		}
	}
	GameConfig AGNetwork::getGameConfig() const noexcept
	{
		return game_config;
	}
	ml::Event AGNetwork::addEvent() const
	{
		return graph.context().createEvent();
	}
	/*
	 * private
	 */
	ml::Shape AGNetwork::get_input_shape() const noexcept
	{
		return ml::Shape( { getBatchSize(), game_config.rows, game_config.cols, 1 });
	}
	void AGNetwork::copy_input(const NetworkDataPack &pack)
	{
		if (graph.device().isCPU())
			ml::unpackInput(graph.context(), graph.getInput(), pack.getInput());
		else
		{
			input_on_device.copyFrom(graph.context(), pack.getInput());
			ml::unpackInput(graph.context(), graph.getInput(), input_on_device);
		}
	}
	std::unique_ptr<AGNetwork> createAGNetwork(const std::string &architecture)
	{
		static const ResnetPV resnet_pv;
		static const ResnetPVraw resnet_pv_raw;
		static const ResnetPVQ resnet_pvq;
		static const BottleneckPV bottleneck_pv;
		static const BottleneckPVraw bottleneck_pv_raw;
		static const BottleneckBroadcastPVraw bottleneck_broadcast_pv_raw;
		static const BottleneckPoolingPVraw bottleneck_pooling_pv_raw;
		static const BottleneckPVQ bottleneck_pvq;
		static const ResnetPVQraw resnet_pvq_raw;
		static const ResnetOld resnet_old;

		static const Transformer_v2 transformer_v2;

		static const ConvUnet conv_unet;
		static const TransformerUnet transformer_unet;

		static const ResnetPVraw_v0 resnet_pv_raw_v0;
		static const ResnetPVraw_v1 resnet_pv_raw_v1;
		static const ResnetPVraw_v2 resnet_pv_raw_v2;

		static const BottleneckPVUM bottleneck_pvum;
		static const ConvNextPVraw convnext_pv_raw;
		static const ConvNextPVQraw convnext_pvq_raw;

		if (architecture == resnet_pv.name())
			return std::make_unique<ResnetPV>();
		if (architecture == resnet_pv_raw.name())
			return std::make_unique<ResnetPVraw>();
		if (architecture == resnet_pvq.name())
			return std::make_unique<ResnetPVQ>();
		if (architecture == bottleneck_pv.name())
			return std::make_unique<BottleneckPV>();
		if (architecture == bottleneck_pv_raw.name())
			return std::make_unique<BottleneckPVraw>();
		if (architecture == bottleneck_broadcast_pv_raw.name())
			return std::make_unique<BottleneckBroadcastPVraw>();
		if (architecture == bottleneck_pooling_pv_raw.name())
			return std::make_unique<BottleneckPoolingPVraw>();
		if (architecture == bottleneck_pvq.name())
			return std::make_unique<BottleneckPVQ>();
		if (architecture == resnet_pvq_raw.name())
			return std::make_unique<ResnetPVQraw>();
		if (architecture == resnet_old.name())
			return std::make_unique<ResnetOld>();

		if (architecture == transformer_v2.name())
			return std::make_unique<Transformer_v2>();

		if (architecture == conv_unet.name())
			return std::make_unique<ConvUnet>();
		if (architecture == transformer_unet.name())
			return std::make_unique<TransformerUnet>();

		if (architecture == resnet_pv_raw_v0.name())
			return std::make_unique<ResnetPVraw_v0>();
		if (architecture == resnet_pv_raw_v1.name())
			return std::make_unique<ResnetPVraw_v1>();
		if (architecture == resnet_pv_raw_v2.name())
			return std::make_unique<ResnetPVraw_v2>();

		if (architecture == bottleneck_pvum.name())
			return std::make_unique<BottleneckPVUM>();

		if (architecture == convnext_pv_raw.name())
			return std::make_unique<ConvNextPVraw>();
		if (architecture == convnext_pvq_raw.name())
			return std::make_unique<ConvNextPVQraw>();

		throw std::runtime_error("createAGNetwork() : unknown architecture '" + architecture + "'");
	}
	std::unique_ptr<AGNetwork> loadAGNetwork(const std::string &path)
	{
		FileLoader fl(path);
		std::unique_ptr<AGNetwork> result = createAGNetwork(fl.getJson()["architecture"]);

		result->loadFrom(fl.getJson(), fl.getBinaryData());
		return result;
	}

} /* namespace ag */

