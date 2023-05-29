/*
 * AGNetwork.cpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <alphagomoku/networks/networks.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/Tensor.hpp>
#include <minml/core/math.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

namespace
{
	int get_index(std::initializer_list<int> idx, const ml::Tensor &t)
	{
		assert(idx.size() <= static_cast<size_t>(t.rank()));
		int result = 0;
		for (size_t i = 0; i < idx.size(); i++)
		{
			const int tmp = idx.begin()[i];
			assert(tmp >= 0 && tmp < t.dim(i));
			result += tmp * t.stride(i);
		}
		return result;
	}

	void* get_pointer(ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * get_index(idx, src);
	}
	const void* get_pointer(const ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<const uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * get_index(idx, src);
	}

	ml::Shape get_encoding_shape(const ml::Shape &input_shape)
	{
		assert(input_shape.rank() == 4);
		return ml::Shape( { input_shape[0], input_shape[1], input_shape[2], 1 });
	}

	ag::float3 to_float3(const ag::Value &value) noexcept
	{
		return ag::float3 { value.win_rate, value.draw_rate, value.loss_rate() };
	}
	ag::Value from_float3(const ag::float3 &f) noexcept
	{
		return ag::Value(f.x, f.y);
	}
}

namespace ag
{

	AGNetwork::AGNetwork() noexcept
	{
	}
	std::vector<float> AGNetwork::getAccuracy(int batchSize, int top_k) const
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(game_config.rows, game_config.cols);
		matrix<float> answer(game_config.rows, game_config.cols);
		for (int b = 0; b < batchSize; b++)
		{
			std::memcpy(output.data(), get_pointer(outputs_on_cpu.at(POLICY_OUTPUT_INDEX), { b, 0, 0, 0 }), output.sizeInBytes());
			std::memcpy(answer.data(), get_pointer(targets_on_cpu.at(POLICY_OUTPUT_INDEX), { b, 0, 0, 0 }), output.sizeInBytes());

			Move correct = pickMove(answer);
			for (int l = 0; l < top_k; l++)
			{
				Move best = pickMove(output);
				if (correct == best)
					for (int m = l; m < top_k; m++)
						result.at(1 + m) += 1;
				output.at(best.row, best.col) = 0.0f;
			}
			result.at(0) += 1;
		}
		return result;
	}

	void AGNetwork::packInputData(int index, const matrix<Sign> &board, Sign signToMove)
	{
		get_pattern_calculator().setBoard(board, signToMove);
		input_features.encode(get_pattern_calculator());
		packInputData(index, input_features);
	}
	void AGNetwork::packInputData(int index, const NNInputFeatures &features)
	{
		assert(index >= 0 && index < getBatchSize());

		std::memcpy(get_pointer(*input_on_cpu, { index }), features.data(), features.sizeInBytes());
	}
	void AGNetwork::packTargetData(int index, const matrix<float> &policy, const matrix<Value> &actionValues, Value value)
	{
		assert(index >= 0 && index < getBatchSize());

		std::memcpy(get_pointer(targets_on_cpu.at(POLICY_OUTPUT_INDEX), { index }), policy.data(), policy.sizeInBytes());

		const float3 tmp = to_float3(value);
		std::memcpy(get_pointer(targets_on_cpu.at(VALUE_OUTPUT_INDEX), { index }), &tmp, sizeof(tmp));

		if (targets_on_cpu.size() == 3)
		{
			workspace.resize(game_config.rows * game_config.cols);
			for (int i = 0; i < actionValues.size(); i++)
				workspace[i] = to_float3(actionValues[i]);
			std::memcpy(get_pointer(targets_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX), { index }), workspace.data(), sizeof(float3) * workspace.size());
		}
	}
	void AGNetwork::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const
	{
		assert(index >= 0 && index < getBatchSize());

		ml::convertType(context_on_cpu, policy.data(), ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(POLICY_OUTPUT_INDEX), { index }),
				outputs_on_cpu.at(POLICY_OUTPUT_INDEX).dtype(), policy.size());

		float3 tmp;
		ml::convertType(context_on_cpu, &tmp, ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(VALUE_OUTPUT_INDEX), { index }),
				outputs_on_cpu.at(VALUE_OUTPUT_INDEX).dtype(), 3);
		value = from_float3(tmp);

		if (outputs_on_cpu.size() == 3)
		{
			workspace.resize(game_config.rows * game_config.cols);
			ml::convertType(context_on_cpu, workspace.data(), ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX), {
					index }), outputs_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX).dtype(), 3 * workspace.size());
			for (int i = 0; i < actionValues.size(); i++)
				actionValues[i] = from_float3(workspace[i]);
		}
	}

	void AGNetwork::asyncForwardLaunch(int batch_size)
	{
		pack_input_to_graph(batch_size);

		graph.forward(batch_size);

		assert(outputs_on_cpu.size() == (size_t )graph.numberOfOutputs());
		for (int i = 0; i < graph.numberOfOutputs(); i++)
			outputs_on_cpu.at(i).copyFrom(graph.context(), graph.getOutput(i));
	}
	void AGNetwork::asyncForwardJoin()
	{
		graph.context().synchronize();
	}

	void AGNetwork::forward(int batch_size)
	{
		pack_input_to_graph(batch_size);
		graph.forward(batch_size);
		assert(outputs_on_cpu.size() == (size_t )graph.numberOfOutputs());
		for (int i = 0; i < graph.numberOfOutputs(); i++)
			outputs_on_cpu.at(i).copyFrom(graph.context(), graph.getOutput(i));
		graph.context().synchronize();
	}
	void AGNetwork::backward(int batch_size)
	{
		assert(graph.isTrainable());
		assert(outputs_on_cpu.size() == (size_t )graph.numberOfOutputs());
		for (int i = 0; i < graph.numberOfOutputs(); i++)
			graph.getTarget(i).copyFrom(graph.context(), targets_on_cpu.at(i));

		graph.backward(batch_size);
		graph.learn();
		graph.context().synchronize();
	}
	std::vector<float> AGNetwork::getLoss(int batch_size)
	{
		assert(graph.isTrainable());
		assert(outputs_on_cpu.size() == (size_t )graph.numberOfOutputs());
		for (int i = 0; i < graph.numberOfOutputs(); i++)
			graph.getTarget(i).copyFrom(graph.context(), targets_on_cpu.at(i));
		return graph.getLoss(batch_size);
	}

	void AGNetwork::changeLearningRate(float lr)
	{
		assert(graph.isTrainable());
		graph.setLearningRate(lr);
	}
	void AGNetwork::optimize()
	{
		graph.makeNonTrainable();
		ml::FoldBatchNorm().optimize(graph);
		ml::FoldAdd().optimize(graph);
	}
	void AGNetwork::convertToHalfFloats()
	{
		ml::DataType new_type = ml::DataType::FLOAT32;
		if (graph.device().supportsType(ml::DataType::FLOAT16))
			new_type = ml::DataType::FLOAT16;
		else
		{
			if (graph.device().supportsType(ml::DataType::BFLOAT16) and graph.device().isCPU()) // on gpus bf16 can be slower than fp16 (and has worse accuracy)
				new_type = ml::DataType::BFLOAT16;
		}
		if (new_type != ml::DataType::FLOAT32)
		{
			graph.convertTo(new_type);
			reallocate_tensors();
		}
	}
	void AGNetwork::init(const GameConfig &gameOptions, const TrainingConfig &trainingOptions)
	{
		game_config = gameOptions;
		create_network(trainingOptions);
		reallocate_tensors();
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
		reallocate_tensors();
	}
	ml::Shape AGNetwork::getInputShape() const
	{
		return graph.getInputShape();
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
			input_on_cpu = nullptr;
			outputs_on_cpu.clear();
			targets_on_cpu.clear();
		}
	}
	/*
	 * private
	 */
	void AGNetwork::pack_input_to_graph(int batch_size)
	{
		ml::Shape shape = graph.getInputShape();
		shape[0] = batch_size;
		if (graph.device().isCPU())
			ml::unpackInput(graph.context(), graph.getInput(), *input_on_cpu);
		else
		{
			input_on_device->copyFrom(graph.context(), *input_on_cpu);
			ml::unpackInput(graph.context(), graph.getInput(), *input_on_device);
		}
	}
	PatternCalculator& AGNetwork::get_pattern_calculator()
	{
		if (pattern_calculator == nullptr)
		{
			pattern_calculator = std::make_unique<PatternCalculator>(game_config);
			input_features = NNInputFeatures(game_config.rows, game_config.cols);
		}
		return *pattern_calculator;
	}
	void AGNetwork::reallocate_tensors()
	{
		const ml::DataType dtype = graph.dtype();

		const ml::Shape encoding_shape = get_encoding_shape(graph.getInputShape());
		input_on_cpu = std::make_unique<ml::Tensor>(encoding_shape, ml::DataType::INT32, ml::Device::cpu());
		if (not graph.device().isCPU())
			input_on_device = std::make_unique<ml::Tensor>(encoding_shape, ml::DataType::INT32, graph.device());

		outputs_on_cpu.clear();
		for (int i = 0; i < graph.numberOfOutputs(); i++)
			outputs_on_cpu.push_back(ml::Tensor(graph.getOutputShape(i), dtype, ml::Device::cpu()));

		if (graph.isTrainable())
		{
			targets_on_cpu.clear();
			for (int i = 0; i < graph.numberOfOutputs(); i++)
				targets_on_cpu.push_back(ml::Tensor(graph.getOutputShape(i), dtype, ml::Device::cpu()));
		}

		if (graph.device().isCUDA() and not input_on_cpu->isPageLocked())
		{
			input_on_cpu->pageLock();
			for (size_t i = 0; i < outputs_on_cpu.size(); i++)
				outputs_on_cpu.at(i).pageLock();

			if (graph.isTrainable())
			{
				for (size_t i = 0; i < outputs_on_cpu.size(); i++)
					targets_on_cpu.at(i).pageLock();
			}
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
		static const BottleneckPVQ bottleneck_pvq;
		static const ResnetPVQraw resnet_pvq_raw;
		static const ResnetOld resnet_old;

		static const ResnetPVraw_v0 resnet_pv_raw_v0;
		static const ResnetPVraw_v1 resnet_pv_raw_v1;
		static const ResnetPVraw_v2 resnet_pv_raw_v2;

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
		if (architecture == bottleneck_pvq.name())
			return std::make_unique<BottleneckPVQ>();
		if (architecture == resnet_pvq_raw.name())
			return std::make_unique<ResnetPVQraw>();
		if (architecture == resnet_old.name())
			return std::make_unique<ResnetOld>();
		if (architecture == resnet_pv_raw_v0.name())
			return std::make_unique<ResnetPVraw_v0>();
		if (architecture == resnet_pv_raw_v1.name())
			return std::make_unique<ResnetPVraw_v1>();
		if (architecture == resnet_pv_raw_v2.name())
			return std::make_unique<ResnetPVraw_v2>();

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

