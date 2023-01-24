/*
 * QHeadSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/QHeadSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{

	QHeadSelector::QHeadSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> QHeadSelector::clone() const
	{
		return std::make_unique<QHeadSelector>(exploration_constant, style_factor);
	}
	Edge* QHeadSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // use action value head
				const float U = edge->getPolicyPrior() * sqrt_visit / (1.0f + edge->getVisits()); // classical PUCT formula

				if (Q + U > bestValue)
				{
					selected = edge;
					bestValue = Q + U;
				}
			}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

} /* namespace ag */
