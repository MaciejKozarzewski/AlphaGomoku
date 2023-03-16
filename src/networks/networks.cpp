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

} /* namespace ag */

