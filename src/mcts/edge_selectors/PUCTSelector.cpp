/*
 * PUCTSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/PUCTSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{

	PUCTSelector::PUCTSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> PUCTSelector::clone() const
	{
		return std::make_unique<PUCTSelector>(exploration_constant, style_factor);
	}
	Edge* PUCTSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const float parent_value = getQ(node, style_factor);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());
//		const float cbrt_visit = exploration_constant * cbrtf(node->getVisits());
//		const float log_visit = exploration_constant * logf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
				const float Q = (edge->getVisits() > 0) ? getQ(edge, style_factor) * getVirtualLoss(edge) : parent_value; // init to parent
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
