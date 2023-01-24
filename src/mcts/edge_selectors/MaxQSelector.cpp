/*
 * MaxQSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/MaxValueSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{
	MaxValueSelector::MaxValueSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> MaxValueSelector::clone() const
	{
		return std::make_unique<MaxValueSelector>(style_factor);
	}
	Edge* MaxValueSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			const float Q = getQ(edge, style_factor) + 1.0e6f * getProvenQ(edge);
			if (Q > bestValue)
			{
				selected = edge;
				bestValue = Q;
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

} /* namespace ag */
