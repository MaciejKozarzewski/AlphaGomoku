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

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/GlobalBroadcastHW.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/Softmax.hpp>
#include <minml/layers/Parameter.hpp>
#include <minml/training/Optimizer.hpp>
#include <minml/training/Regularizer.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/Tensor.hpp>
#include <minml/core/math.hpp>

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
}

namespace ag
{
	AGNetwork::AGNetwork(const GameConfig &gameOptions) :
			game_config(gameOptions)
	{
	}
	AGNetwork::AGNetwork(const GameConfig &gameOptions, const std::string &path) :
			game_config(gameOptions)
	{
		loadFromFile(path);
	}
	AGNetwork::AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions) :
			game_config(gameOptions)
	{
		create_network(trainingOptions);
		reallocate_tensors();
	}

	std::vector<float> AGNetwork::getAccuracy(int batchSize, int top_k) const
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(game_config.rows, game_config.cols);
		matrix<float> answer(game_config.rows, game_config.cols);
		for (int b = 0; b < batchSize; b++)
		{
			std::memcpy(output.data(), get_pointer(*policy_on_cpu, { b, 0, 0, 0 }), output.sizeInBytes());
			std::memcpy(answer.data(), get_pointer(*policy_target_on_cpu, { b, 0, 0, 0 }), output.sizeInBytes());

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
		getPatternCalculator().setBoard(board, signToMove);
		input_features.encode(getPatternCalculator());
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

		std::memcpy(get_pointer(*policy_target_on_cpu, { index }), policy.data(), policy.sizeInBytes());

		float tmp[3] = { value.win_rate, value.draw_rate, value.loss_rate() };
		std::memcpy(get_pointer(*value_target_on_cpu, { index }), tmp, sizeof(tmp));

		workspace.resize(3 * game_config.rows * game_config.cols);
		for (int i = 0; i < actionValues.size(); i++)
		{
			workspace[3 * i + 0] = actionValues[i].win_rate;
			workspace[3 * i + 1] = actionValues[i].draw_rate;
			workspace[3 * i + 2] = actionValues[i].loss_rate();
		}
		std::memcpy(get_pointer(*action_values_target_on_cpu, { index }), workspace.data(), sizeof(float) * workspace.size());
	}
	void AGNetwork::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const
	{
		assert(index >= 0 && index < getBatchSize());

		ml::convertType(context_on_cpu, policy.data(), ml::DataType::FLOAT32, get_pointer(*policy_on_cpu, { index }), policy_on_cpu->dtype(),
				policy.size());
		float tmp[3];
		ml::convertType(context_on_cpu, &tmp, ml::DataType::FLOAT32, get_pointer(*value_on_cpu, { index }), value_on_cpu->dtype(), 3);
		value = Value(tmp[0], tmp[1]);

		workspace.resize(3 * game_config.rows * game_config.cols);
		ml::convertType(context_on_cpu, workspace.data(), ml::DataType::FLOAT32, get_pointer(*action_values_on_cpu, { index }),
				action_values_on_cpu->dtype(), workspace.size());
		for (int i = 0; i < actionValues.size(); i++)
			actionValues[i] = Value(workspace[3 * i], workspace[3 * i + 1]);
	}

	void AGNetwork::asyncForwardLaunch(int batch_size)
	{
		pack_input_to_graph(batch_size);

		graph.forward(batch_size);

		policy_on_cpu->copyFrom(graph.context(), graph.getOutput(0));
		value_on_cpu->copyFrom(graph.context(), graph.getOutput(1));
		action_values_on_cpu->copyFrom(graph.context(), graph.getOutput(2));
	}
	void AGNetwork::asyncForwardJoin()
	{
		graph.context().synchronize();
	}

	void AGNetwork::forward(int batch_size)
	{
		pack_input_to_graph(batch_size);
		graph.forward(batch_size);
		policy_on_cpu->copyFrom(graph.context(), graph.getOutput(0));
		value_on_cpu->copyFrom(graph.context(), graph.getOutput(1));
		action_values_on_cpu->copyFrom(graph.context(), graph.getOutput(2));
		graph.context().synchronize();
	}
	void AGNetwork::backward(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target_on_cpu);
		graph.getTarget(1).copyFrom(graph.context(), *value_target_on_cpu);
		graph.getTarget(2).copyFrom(graph.context(), *action_values_target_on_cpu);

		graph.backward(batch_size);
		graph.learn();
		graph.context().synchronize();
	}
	std::vector<float> AGNetwork::getLoss(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target_on_cpu);
		graph.getTarget(1).copyFrom(graph.context(), *value_target_on_cpu);
		graph.getTarget(2).copyFrom(graph.context(), *action_values_target_on_cpu);
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
	void AGNetwork::saveToFile(const std::string &path) const
	{
		graph.context().synchronize();
		SerializedObject so;
		Json json = graph.save(so);
		FileSaver fs(path);
		fs.save(json, so, 2);
	}
	void AGNetwork::loadFromFile(const std::string &path)
	{
		graph.context().synchronize();
		FileLoader fl(path);
		graph.load(fl.getJson(), fl.getBinaryData());
		game_config.rows = graph.getInputShape()[1];
		game_config.cols = graph.getInputShape()[2];
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
			policy_on_cpu = nullptr;
			value_on_cpu = nullptr;
			action_values_on_cpu = nullptr;
			policy_target_on_cpu = nullptr;
			value_target_on_cpu = nullptr;
			action_values_target_on_cpu = nullptr;
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
	PatternCalculator& AGNetwork::getPatternCalculator()
	{
		if (pattern_calculator == nullptr)
		{
			pattern_calculator = std::make_unique<PatternCalculator>(game_config);
			input_features = NNInputFeatures(game_config.rows, game_config.cols);
		}
		return *pattern_calculator;
	}
	void AGNetwork::create_network(const TrainingConfig &trainingOptions)
	{
		int batch_size = trainingOptions.device_config.batch_size;
		int blocks = trainingOptions.blocks;
		int filters = trainingOptions.filters;

		auto x = graph.addInput( { batch_size, game_config.rows, game_config.cols, 32 });
		x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
//		x = graph.add(ml::GlobalBroadcastHW(), x);

		for (int i = 0; i < blocks; i++)
		{
//			auto y = graph.add(ml::Conv2D(filters / 2, 1, "linear").useBias(false), x);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//
//			y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), y);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//
//			y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), y);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//
//			y = graph.add(ml::Conv2D(filters, 1, "linear").useBias(false), y);
//			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

			auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

			y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

			x = graph.add(ml::Add("relu"), { x, y });
//			if ((i + 1) % 5 == 0)
//				x = graph.add(ml::GlobalBroadcastHW(), x);
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);

		p = graph.add(ml::Conv2D(1, 1, "linear"), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(4, 1, "linear").useBias(false), x);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);

		v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Dense(3, "linear"), v);
		v = graph.add(ml::Softmax( { 1 }), v);
		graph.addOutput(v);

		// action values head
		auto q = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		q = graph.add(ml::BatchNormalization("tanh").useGamma(false), q);

		q = graph.add(ml::Conv2D(3, 1, "linear"), q);
		q = graph.add(ml::Softmax( { 3 }), q);
		graph.addOutput(q, 0.1f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
		{
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
			graph.getNode(graph.numberOfNodes() - 2).getLayer().setRegularizer(ml::Regularizer(0.1f * trainingOptions.l2_regularization));
		}
		graph.moveTo(trainingOptions.device_config.device);
	}
	void AGNetwork::reallocate_tensors()
	{
		const ml::DataType dtype = graph.dtype();

		const ml::Shape encoding_shape = get_encoding_shape(graph.getInputShape());
		input_on_cpu = std::make_unique<ml::Tensor>(encoding_shape, ml::DataType::INT32, ml::Device::cpu());
		if (not graph.device().isCPU())
			input_on_device = std::make_unique<ml::Tensor>(encoding_shape, ml::DataType::INT32, graph.device());

		policy_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(0), dtype, ml::Device::cpu());
		value_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(1), dtype, ml::Device::cpu());
		action_values_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(2), dtype, ml::Device::cpu());

		if (graph.isTrainable())
		{
			policy_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(0), ml::DataType::FLOAT32, ml::Device::cpu());
			value_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(1), ml::DataType::FLOAT32, ml::Device::cpu());
			action_values_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(2), ml::DataType::FLOAT32, ml::Device::cpu());
		}

		if (graph.device().isCUDA() and not input_on_cpu->isPageLocked())
		{
			input_on_cpu->pageLock();
			policy_on_cpu->pageLock();
			value_on_cpu->pageLock();
			action_values_on_cpu->pageLock();

			if (graph.isTrainable())
			{
				policy_target_on_cpu->pageLock();
				value_target_on_cpu->pageLock();
				action_values_target_on_cpu->pageLock();
			}
		}
	}
}
