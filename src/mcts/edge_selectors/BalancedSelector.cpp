/*
 * BalancedSelector.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_selectors/BalancedSelector.hpp>
#include <alphagomoku/mcts/edge_selectors/selector_utils.hpp>
#include <alphagomoku/mcts/Node.hpp>

namespace ag
{
	BalancedSelector::BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector) :
			balance_depth(balanceDepth),
			base_selector(baseSelector.clone())
	{
	}
	std::unique_ptr<EdgeSelector> BalancedSelector::clone() const
	{
		return std::make_unique<BalancedSelector>(balance_depth, *base_selector);
	}
	Edge* BalancedSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->getDepth() < balance_depth)
		{
			Edge *selected = nullptr;
			float bestValue = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
			{
				const float Q = getBalance(edge) * getVirtualLoss(edge);
				if (Q > bestValue)
				{
					selected = edge;
					bestValue = Q;
				}
			}
			assert(selected != nullptr); // there should always be some best edge
			return selected;
		}
		else
			return base_selector->select(node);
	}

} /* namespace ag */

