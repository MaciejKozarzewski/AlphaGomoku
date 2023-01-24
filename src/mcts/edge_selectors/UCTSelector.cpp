/*
 * UCTSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/UCTSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{
	UCTSelector::UCTSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> UCTSelector::clone() const
	{
		return std::make_unique<UCTSelector>(exploration_constant, style_factor);
	}
	Edge* UCTSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
//		const float parent_value = getQ(node, style_factor);
		const float log_visit = logf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // init to loss
				const float U = exploration_constant * sqrtf(log_visit / (1.0f + edge->getVisits()));
				const float P = edge->getPolicyPrior() / (1.0f + edge->getVisits());

				if (Q + U + P > bestValue)
				{
					selected = edge;
					bestValue = Q + U + P;
				}
			}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

} /* namespace ag */
