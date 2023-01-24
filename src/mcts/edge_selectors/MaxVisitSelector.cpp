/*
 * MaxVisitSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/MaxVisitSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{
	std::unique_ptr<EdgeSelector> MaxVisitSelector::clone() const
	{
		return std::make_unique<MaxVisitSelector>();
	}
	Edge* MaxVisitSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if (edge->getVisits() > bestValue)
			{
				selected = edge;
				bestValue = edge->getVisits();
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

} /* namespace ag */
