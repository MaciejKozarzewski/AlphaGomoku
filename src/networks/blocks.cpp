/*
 * blocks.cpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/blocks.hpp>

#include <minml/layers/ChannelAveragePooling.hpp>
#include <minml/layers/ChannelScaling.hpp>
#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/DepthwiseConv2D.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/GlobalAveragePooling.hpp>
#include <minml/layers/LearnableGlobalPooling.hpp>
#include <minml/layers/Softmax.hpp>
#include <minml/layers/SqueezeAndExcitation.hpp>
#include <minml/layers/SpatialScaling.hpp>
#include <minml/layers/PositionalEncoding.hpp>
#include <minml/layers/RMSNormalization.hpp>
#include <minml/layers/MultiHeadAttention.hpp>

namespace ag
{
	ml::GraphNodeID createInputBlock(ml::Graph &graph, const ml::Shape &shape, int filters)
	{
		ml::GraphNodeID x = graph.addInput(shape);
		x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		return x;
	}

	ml::GraphNodeID createPoolingBlock(ml::Graph &graph, ml::GraphNodeID x)
	{
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
	ml::GraphNodeID createBottleneckBlock_v1(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

		x = graph.add(ml::Add("relu"), { x, y });
		return x;
	}
	ml::GraphNodeID createBottleneckBlock_v2(ml::Graph &graph, ml::GraphNodeID x, int filters)
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
	ml::GraphNodeID createBottleneckBlock_v3(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::Conv2D(filters / 2, 1, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters / 2, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

		y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
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

	ml::GraphNodeID squeeze_and_excitation_block(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::GlobalAveragePooling().quantizable(false), x);
		y = graph.add(ml::Dense(filters, "relu").quantizable(false), y);
		y = graph.add(ml::Dense(filters, "sigmoid").quantizable(false), y);
		x = graph.add(ml::ChannelScaling(), { x, y });
		return x;
	}
	ml::GraphNodeID squeeze_and_excitation_block_v2(ml::Graph &graph, ml::GraphNodeID x, int filters)
	{
		auto y = graph.add(ml::GlobalAveragePooling().quantizable(false), x);
		y = graph.add(ml::Dense(filters, "sigmoid").quantizable(false), y);
		x = graph.add(ml::ChannelScaling(), { x, y });
		return x;
	}
	ml::GraphNodeID spatial_scaling_block(ml::Graph &graph, ml::GraphNodeID x, int hw)
	{
		auto y = graph.add(ml::ChannelAveragePooling().quantizable(false), x);
		y = graph.add(ml::Dense(128, "relu").quantizable(false), y);
		y = graph.add(ml::Dense(hw, "sigmoid").quantizable(false), y);
		x = graph.add(ml::SpatialScaling(), { x, y });
		return x;
	}
	ml::GraphNodeID spatial_scaling_block_v2(ml::Graph &graph, ml::GraphNodeID x, int hw)
	{
		auto y = graph.add(ml::Conv2D(1, 1).quantizable(false), x);
		y = graph.add(ml::Dense(hw, "sigmoid").quantizable(false), y);
		x = graph.add(ml::SpatialScaling(), { x, y });
		return x;
	}

	ml::GraphNodeID conv_bn_relu(ml::Graph &graph, ml::GraphNodeID x, int filters, int kernel_size)
	{
		x = graph.add(ml::Conv2D(filters, kernel_size, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		return x;
	}
	ml::GraphNodeID conv_bn(ml::Graph &graph, ml::GraphNodeID x, int filters, int kernel_size)
	{
		x = graph.add(ml::Conv2D(filters, kernel_size, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("linear").useGamma(false), x);
		return x;
	}
	ml::GraphNodeID mha_pre_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding, int head_dim, int range, bool symmetric)
	{
		auto y = graph.add(ml::RMSNormalization(false), x);
		y = graph.add(ml::PositionalEncoding(), y);
		y = graph.add(ml::Conv2D((3 - symmetric) * embedding, 1).useBias(false), y);
		y = graph.add(ml::MultiHeadAttention(embedding / head_dim, range, symmetric), y);
		y = graph.add(ml::Conv2D(embedding, 1), y);
		x = graph.add(ml::Add(), { x, y });
		return x;
	}
	ml::GraphNodeID ffn_pre_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding)
	{
		auto y = graph.add(ml::RMSNormalization(false), x);
		y = graph.add(ml::Conv2D(embedding, 1, "relu"), y);
		y = graph.add(ml::Conv2D(embedding, 1), y);
		x = graph.add(ml::Add(), { x, y });
		return x;
	}

	ml::GraphNodeID mha_post_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding, int head_dim, int range, bool symmetric)
	{
		auto y = graph.add(ml::PositionalEncoding(), x);
		y = graph.add(ml::Conv2D((3 - symmetric) * embedding, 1).useBias(false), y);
		y = graph.add(ml::MultiHeadAttention(embedding / head_dim, range, symmetric), y);
		y = graph.add(ml::Conv2D(embedding, 1), y);
		y = graph.add(ml::RMSNormalization(false), y);
		x = graph.add(ml::Add(), { x, y });
		return x;
	}
	ml::GraphNodeID ffn_post_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding)
	{
		auto y = graph.add(ml::Conv2D(embedding, 1, "relu"), x);
		y = graph.add(ml::Conv2D(embedding, 1), y);
		y = graph.add(ml::RMSNormalization(false), y);
		x = graph.add(ml::Add(), { x, y });
		return x;
	}

} /* namespace ag */

