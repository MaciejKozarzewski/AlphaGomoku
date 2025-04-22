/*
 * blocks.hpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_BLOCKS_HPP_
#define ALPHAGOMOKU_NETWORKS_BLOCKS_HPP_

#include <minml/graph/Graph.hpp>

namespace ag
{

	ml::GraphNodeID createInputBlock(ml::Graph &graph, const ml::Shape &shape, int filters);

	ml::GraphNodeID createPoolingBlock(ml::Graph &graph, ml::GraphNodeID x);
	ml::GraphNodeID createResidualBlock(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createBottleneckBlock_v1(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createBottleneckBlock_v2(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createBottleneckBlock_v3(ml::Graph &graph, ml::GraphNodeID x, int filters);

	ml::GraphNodeID createPolicyHead(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createValueHead(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createActionValuesHead(ml::Graph &graph, ml::GraphNodeID x, int filters);

	ml::GraphNodeID squeeze_and_excitation_block(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID squeeze_and_excitation_block_v2(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID spatial_scaling_block(ml::Graph &graph, ml::GraphNodeID x, int hw);
	ml::GraphNodeID spatial_scaling_block_v2(ml::Graph &graph, ml::GraphNodeID x, int hw);

	ml::GraphNodeID conv_bn_relu(ml::Graph &graph, ml::GraphNodeID x, int filters, int kernel_size);
	ml::GraphNodeID conv_bn(ml::Graph &graph, ml::GraphNodeID x, int filters, int kernel_size);
	ml::GraphNodeID mha_pre_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding, int head_dim, int range, bool symmetric);
	ml::GraphNodeID ffn_pre_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding);

	ml::GraphNodeID mha_post_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding, int head_dim, int range, bool symmetric);
	ml::GraphNodeID ffn_post_norm_block(ml::Graph &graph, ml::GraphNodeID x, int embedding);

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_BLOCKS_HPP_ */
