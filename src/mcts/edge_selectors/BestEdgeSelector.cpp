/*
 * BestEdgeSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/BestEdgeSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{

	BestEdgeSelector::BestEdgeSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> BestEdgeSelector::clone() const
	{
		return std::make_unique<BestEdgeSelector>(style_factor);
	}
	Edge* BestEdgeSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = nullptr;
		double bestValue = std::numeric_limits<double>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			double value = edge->getVisits() + getQ(edge, style_factor) * node->getVisits() + 0.001 * edge->getPolicyPrior()
					+ getProvenQ(edge) * 1.0e9;
			if (value > bestValue)
			{
				selected = edge;
				bestValue = value;
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

} /* namespace ag */
