/*
 * TreePolicy.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/TreePolicy.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <cassert>

namespace
{
	template<typename T>
	float getQ(const T *n, float c) noexcept
	{
		return n->getWinRate() + c * n->getDrawRate();
	}
	template<typename T>
	float getProvenQ(const T *n) noexcept
	{
		return static_cast<int>(n->getProvenValue() == ag::ProvenValue::WIN) - static_cast<int>(n->getProvenValue() == ag::ProvenValue::LOSS);
	}
	template<typename T>
	float getBalance(const T *n) noexcept
	{
		return -fabsf(n->getWinRate() - (1.0f - n->getWinRate() - n->getDrawRate()));
	}
}

namespace ag
{
	PuctPolicy::PuctPolicy(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	PuctPolicy* PuctPolicy::clone() const
	{
		return new PuctPolicy(exploration_constant, style_factor);
	}
	Edge* PuctPolicy::select(const Node *node) noexcept
	{
		assert(node->getVisits() > 0);
		const float parent_value = 1.0f - getQ(node, style_factor);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());

		Edge *selected = node->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q = (edge->getVisits() == 0) ? parent_value : (getQ(edge, style_factor) + getProvenQ(edge));
			float U = edge->getPolicyPrior() * sqrt_visit / (1.0f + edge->getVisits()); // classical PUCT formula

			if (Q + U > bestValue)
			{
				selected = edge;
				bestValue = Q + U;
			}
		}
		assert(selected != node->end()); // this must never happen
		return selected;
	}

	BalancedPolicy::BalancedPolicy(int balanceDepth, const TreePolicy &basePolicy) :
			balance_depth(balanceDepth),
			base_policy(basePolicy.clone())
	{
	}
	BalancedPolicy* BalancedPolicy::clone() const
	{
		return new BalancedPolicy(balance_depth, *base_policy);
	}
	Edge* BalancedPolicy::select(const Node *node) noexcept
	{
		if (node->getDepth() < balance_depth)
		{
			Edge *selected = node->end();
			float bestValue = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
			{
				float Q = getBalance(edge);
				if (Q > bestValue)
				{
					selected = edge;
					bestValue = Q;
				}
			}
			assert(selected != node->end()); // this must never happen
			return selected;
		}
		else
			return base_policy->select(node);
	}
} /* namespace ag */

