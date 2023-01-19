/*
 * AGNetwork.cpp
 *
 *  Created on: Mar 8, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/game/Board.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
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
	AGNetwork::AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions) :
			rows(gameOptions.rows),
			cols(gameOptions.cols)
	{
		create_network(trainingOptions);
		reallocate_tensors();
	}

	std::vector<float> AGNetwork::getAccuracy(int batchSize, int top_k) const
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(rows, cols);
		matrix<float> answer(rows, cols);
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

	void AGNetwork::packInputData(int index, const matrix<uint32_t> &features)
	{
		assert(index >= 0 && index < getBatchSize());

		std::memcpy(get_pointer(*input_on_cpu, { index }), features.data(), features.sizeInBytes());
	}
	void AGNetwork::packTargetData(int index, const matrix<float> &policy, matrix<Value> &actionValues, Value value)
	{
		assert(index >= 0 && index < getBatchSize());

		std::memcpy(get_pointer(*policy_target_on_cpu, { index }), policy.data(), policy.sizeInBytes());
		std::memcpy(get_pointer(*value_target_on_cpu, { index }), &value, sizeof(Value));
//		std::memcpy(get_pointer(*action_values_target_on_cpu, { index }), actionValues.data(), actionValues.sizeInBytes());
	}
	void AGNetwork::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const
	{
		assert(index >= 0 && index < getBatchSize());

		ml::convertType(context_on_cpu, policy.data(), ml::DataType::FLOAT32, get_pointer(*policy_on_cpu, { index }), policy_on_cpu->dtype(),
				policy.size());
		ml::convertType(context_on_cpu, &value, ml::DataType::FLOAT32, get_pointer(*value_on_cpu, { index }), value_on_cpu->dtype(), 3);
//		ml::convertType(context_on_cpu, actionValues.data(), ml::DataType::FLOAT32, get_pointer(*action_values_on_cpu, { index }),
//				action_values_on_cpu->dtype(), actionValues.size());
	}

	void AGNetwork::asyncForwardLaunch(int batch_size)
	{
		pack_input_to_graph(batch_size);
		graph.forward(batch_size);
		policy_on_cpu->copyFrom(graph.context(), graph.getOutput(0));
		value_on_cpu->copyFrom(graph.context(), graph.getOutput(1));
//		action_values_on_cpu->copyFrom(graph.context(), graph.getOutput(2));
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
//		action_values_on_cpu->copyFrom(graph.context(), graph.getOutput(2));
		graph.context().synchronize();
	}
	void AGNetwork::backward(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target_on_cpu);
		graph.getTarget(1).copyFrom(graph.context(), *value_target_on_cpu);
//		graph.getTarget(2).copyFrom(graph.context(), *action_values_target_on_cpu);

		graph.backward(batch_size);
		graph.learn();
		graph.context().synchronize();
	}
	std::vector<float> AGNetwork::getLoss(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target_on_cpu);
		graph.getTarget(1).copyFrom(graph.context(), *value_target_on_cpu);
//		graph.getTarget(2).copyFrom(graph.context(), *action_values_target_on_cpu);
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
		graph.print();
	}
	void AGNetwork::convertToHalfFloats()
	{
		graph.convertTo(ml::DataType::FLOAT16); // TODO first check if float16 is supported, if not we have to use bfloat16 or even float32
		reallocate_tensors();
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
		rows = graph.getInputShape()[1];
		cols = graph.getInputShape()[2];
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
	void AGNetwork::create_network(const TrainingConfig &trainingOptions)
	{
		int batch_size = trainingOptions.device_config.batch_size;
		int blocks = trainingOptions.blocks;
		int filters = trainingOptions.filters;

		auto x = graph.addInput( { batch_size, rows, cols, 32 });
		x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

			y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

			x = graph.add(ml::Add("relu"), { x, y });
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);

		p = graph.add(ml::Conv2D(1, 1, "softmax"), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);

		v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Dense(3, "softmax"), v);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer(trainingOptions.learning_rate));
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
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
//		action_values_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(2), dtype, ml::Device::cpu());

		policy_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(0), ml::DataType::FLOAT32, ml::Device::cpu());
		value_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(1), ml::DataType::FLOAT32, ml::Device::cpu());
//		action_values_target_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(2), ml::DataType::FLOAT32, ml::Device::cpu());

		if (graph.device().isCUDA() and not input_on_cpu->isPageLocked())
		{
			input_on_cpu->pageLock();
			policy_on_cpu->pageLock();
			value_on_cpu->pageLock();
//			action_values_on_cpu->pageLock();

			if (graph.isTrainable())
			{
				policy_target_on_cpu->pageLock();
				value_target_on_cpu->pageLock();
//				action_values_target_on_cpu->pageLock();
			}
		}
	}
}

