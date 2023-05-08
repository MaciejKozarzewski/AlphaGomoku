/*
 * networks.cpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/networks/blocks.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/training/Optimizer.hpp>
#include <minml/training/Regularizer.hpp>
#include <minml/core/Shape.hpp>

#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/Softmax.hpp>

#include <string>

namespace ag
{
	ResnetPV::ResnetPV() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPV::name() const
	{
		return "ResnetPV";
	}
	void ResnetPV::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQ::ResnetPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVQ::name() const
	{
		return "ResnetPVQ";
	}
	void ResnetPVQ::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.05f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPV::BottleneckPV() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPV::name() const
	{
		return "BottleneckPV";
	}
	void BottleneckPV::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVraw::BottleneckPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVraw::name() const
	{
		return "BottleneckPVraw";
	}
	void BottleneckPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckBroadcastPVraw::BottleneckBroadcastPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckBroadcastPVraw::name() const
	{
		return "BottleneckBroadcastPVraw";
	}
	void BottleneckBroadcastPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);
		x = createBroadcastingBlock(graph, x);

		for (int i = 0; i < blocks; i++)
		{
			x = createBottleneckBlock_v3(graph, x, filters);
			if ((i + 1) % 4 == 0)
				x = createBroadcastingBlock(graph, x);
		}

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVQ::BottleneckPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVQ::name() const
	{
		return "BottleneckPVQ";
	}
	void BottleneckPVQ::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.05f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQraw::ResnetPVQraw() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVQraw::name() const
	{
		return "ResnetPVQraw";
	}
	void ResnetPVQraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.2f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
		{
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
			graph.getNode(graph.numberOfNodes() - 2).getLayer().setRegularizer(ml::Regularizer(0.1f * trainingOptions.l2_regularization)); // action values head
		}
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetOld::ResnetOld() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetOld::name() const
	{
		return "ResnetOld";
	}
	ml::Graph& ResnetOld::getGraph()
	{
		return graph;
	}
	void ResnetOld::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = graph.addInput(input_shape);

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
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear").useWeights(false), p);
		p = graph.add(ml::Softmax( { 1 }), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);

		v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Dense(3, "linear"), v);
		v = graph.add(ml::Softmax( { 1 }), v);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v0::ResnetPVraw_v0() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v0::name() const
	{
		return "ResnetPVraw_v0";
	}
	void ResnetPVraw_v0::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v1::ResnetPVraw_v1() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v1::name() const
	{
		return "ResnetPVraw_v1";
	}
	void ResnetPVraw_v1::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v2::ResnetPVraw_v2() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v2::name() const
	{
		return "ResnetPVraw_v2";
	}
	void ResnetPVraw_v2::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);

		p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear").useWeights(false), p);
		p = graph.add(ml::Softmax( { 1 }), p);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

} /* namespace ag */

