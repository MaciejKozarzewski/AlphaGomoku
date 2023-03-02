/*
 * blocks.cpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/blocks.hpp>

#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/Softmax.hpp>

namespace ag
{
	ml::GraphNodeID createInputBlock(ml::Graph &graph, const ml::Shape &shape, int filters)
	{
		ml::GraphNodeID x = graph.addInput(shape);
		x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		return x;
	}

	ml::GraphNodeID createResidualBlock(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

		x = graph.add(ml::Add("relu"), { x, y });
		return x;
	}
	ml::GraphNodeID createBottleneckBlock(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::Conv2D(filters / 2, 1, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters, 1, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

		x = graph.add(ml::Add("relu"), { x, y });
		return x;
	}

	ml::GraphNodeID createPolicyHead(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		x = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = graph.add(ml::Conv2D(1, 1, "linear"), x);
		x = graph.add(ml::Softmax( { 1, 2, 3 }), x);
		return x;
	}
	ml::GraphNodeID createValueHead(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		x = graph.add(ml::Conv2D(4, 1, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		x = graph.add(ml::Dense(3, "linear"), x);
		x = graph.add(ml::Softmax( { 1 }), x);
		return x;
	}
	ml::GraphNodeID createActionValuesHead(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		x = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("tanh").useGamma(false), x);

		x = graph.add(ml::Conv2D(3, 1, "linear"), x);
		x = graph.add(ml::Softmax( { 3 }), x);
		return x;
	}

} /* namespace ag */

