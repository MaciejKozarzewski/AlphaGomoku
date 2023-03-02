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

	ml::GraphNodeID createResidualBlock(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createBottleneckBlock(ml::Graph &graph, ml::GraphNodeID x, int filters);

	ml::GraphNodeID createPolicyHead(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createValueHead(ml::Graph &graph, ml::GraphNodeID x, int filters);
	ml::GraphNodeID createActionValuesHead(ml::Graph &graph, ml::GraphNodeID x, int filters);

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_BLOCKS_HPP_ */
