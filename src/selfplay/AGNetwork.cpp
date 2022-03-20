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
#include <libml/inference/graph_optimizers.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/hardware/DeviceContext.hpp>
#include <libml/layers/conv/Conv2D.hpp>
#include <libml/layers/dense/Affine.hpp>
#include <libml/layers/dense/Dense.hpp>
#include <libml/layers/dense/Flatten.hpp>
#include <libml/layers/merge/Add.hpp>
#include <libml/layers/norm/BatchNormalization.hpp>
#include <libml/layers/activations/Softmax.hpp>
#include <libml/layers/Parameter.hpp>
#include <libml/losses/CrossEntropyLoss.hpp>
#include <libml/losses/KLDivergenceLoss.hpp>
#include <libml/losses/MeanSquareLoss.hpp>
#include <libml/math/activations.hpp>
#include <libml/optimizers/ADAM.hpp>
#include <libml/regularizers/RegularizerL2.hpp>
#include <libml/Scalar.hpp>
#include <libml/Shape.hpp>
#include <libml/Tensor.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

#include <alphagomoku/game/Board.hpp>

namespace ag
{
	AGNetwork::AGNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions) :
			rows(gameOptions.rows),
			cols(gameOptions.cols)
	{
		create_network(trainingOptions);
		allocate_tensors();
	}

	std::vector<float> AGNetwork::getAccuracy(int batchSize, int top_k) const
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(rows, cols);
		matrix<float> answer(rows, cols);
		for (int b = 0; b < batchSize; b++)
		{
			std::memcpy(output.data(), policy_on_cpu->data<float>( { b, 0 }), output.sizeInBytes());
			std::memcpy(answer.data(), policy_target->data<float>( { b, 0 }), output.sizeInBytes());

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
		assert(index >= 0 && index < input_on_cpu->firstDim());
		encodeInputTensor(input_on_cpu->data<float>( { index, 0, 0, 0 }), board, signToMove);
	}
	void AGNetwork::packTargetData(int index, const matrix<float> &policy, matrix<Value> &actionValues, Value value)
	{
		assert(index >= 0 && index < input_on_cpu->firstDim());
		std::memcpy(policy_target->data<float>( { index, 0 }), policy.data(), policy.sizeInBytes());
		// TODO add processing of action values
		value_target->set(value.win, { index, 0 });
		value_target->set(value.draw, { index, 1 });
		value_target->set(value.loss, { index, 2 });
	}
	void AGNetwork::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const
	{
		assert(index >= 0 && index < input_on_cpu->firstDim());
		std::memcpy(policy.data(), policy_on_cpu->data<float>( { index, 0 }), policy.sizeInBytes());
		// TODO add processing of action values
		value = Value(value_on_cpu->get<float>( { index, 0 }), value_on_cpu->get<float>( { index, 1 }), value_on_cpu->get<float>( { index, 2 }));
//		value = Value(value_on_cpu->get<float>( { index, 2 }), value_on_cpu->get<float>( { index, 1 }), value_on_cpu->get<float>( { index, 0 }));
	}

	void AGNetwork::forward(int batch_size)
	{
		graph.getInput().copyFrom(graph.context(), *input_on_cpu);
		graph.forward(batch_size);

		policy_on_cpu->copyFrom(graph.context(), graph.getOutput(0));
		value_on_cpu->copyFrom(graph.context(), graph.getOutput(1));
		graph.context().synchronize();
		ml::math::softmaxForwardInPlace(ml::DeviceContext(), *policy_on_cpu);
		ml::math::softmaxForwardInPlace(ml::DeviceContext(), *value_on_cpu);
	}
	void AGNetwork::backward(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target);
		graph.getTarget(1).copyFrom(graph.context(), *value_target);

		graph.backward(batch_size);
		graph.learn();
		graph.context().synchronize();
	}
	std::vector<ml::Scalar> AGNetwork::getLoss(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget(0).copyFrom(graph.context(), *policy_target);
		graph.getTarget(1).copyFrom(graph.context(), *value_target);
		return graph.getLoss(batch_size);
	}

	void AGNetwork::changeLearningRate(float lr)
	{
		assert(graph.isTrainable());
		for (int i = 0; i < graph.numberOfLayers(); i++)
		{
			graph.getLayer(i).getWeights().getOptimizer().setLearningRate(lr);
			graph.getLayer(i).getBias().getOptimizer().setLearningRate(lr);
		}
	}
	ml::Graph& AGNetwork::getGraph()
	{
		return graph;
	}
	void AGNetwork::optimize()
	{
		graph.makeNonTrainable();
		ml::internal::MergeActivations().optimize(graph);
		ml::internal::BatchNormToAffine().optimize(graph);
		ml::internal::MergeAffine().optimize(graph);
		ml::internal::MergeAdd().optimize(graph);
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
		allocate_tensors();
		rows = graph.getInputShape()[1];
		cols = graph.getInputShape()[2];
	}

	int AGNetwork::getBatchSize() const noexcept
	{
		return graph.getInputShape().firstDim();
	}
	void AGNetwork::setBatchSize(int batchSize)
	{
		graph.setInputShape( { batchSize, rows, cols, 4 });
		input_on_cpu = nullptr;
		policy_target = nullptr;
		allocate_tensors();
	}

	void AGNetwork::create_network(const TrainingConfig &trainingOptions)
	{
		int batch_size = trainingOptions.device_config.batch_size;
		int blocks = trainingOptions.blocks;
		int filters = trainingOptions.filters;

		auto x = graph.addInput( { batch_size, rows, cols, 4 });

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

		p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
		p = graph.add(ml::Flatten(), p);
		p = graph.add(ml::Affine("linear").useWeights(false), p);
		graph.addOutput(p, ml::KLDivergenceLoss().applySoftmax());

		// value head
		auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Flatten(), v);

		v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear"), v);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Dense(3, "linear"), v);
		graph.addOutput(v, ml::CrossEntropyLoss().applySoftmax());

		graph.init();
		graph.setOptimizer(ml::ADAM());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}
	void AGNetwork::allocate_tensors()
	{
		if (input_on_cpu == nullptr)
		{
			input_on_cpu = std::make_unique<ml::Tensor>(graph.getInputShape(), ml::DataType::FLOAT32, ml::Device::cpu());
			policy_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(0), ml::DataType::FLOAT32, ml::Device::cpu());
			value_on_cpu = std::make_unique<ml::Tensor>(graph.getOutputShape(1), ml::DataType::FLOAT32, ml::Device::cpu());
			policy_target = std::make_unique<ml::Tensor>(graph.getOutputShape(0), ml::DataType::FLOAT32, ml::Device::cpu());
			value_target = std::make_unique<ml::Tensor>(graph.getOutputShape(1), ml::DataType::FLOAT32, ml::Device::cpu());

			if (graph.device().isCUDA())
			{
				input_on_cpu->pageLock();
				policy_on_cpu->pageLock();
				value_on_cpu->pageLock();
				policy_target->pageLock();
				value_target->pageLock();
			}
		}
	}
}

