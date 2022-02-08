/*
 * EdgeSelector.cpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <cassert>

namespace
{
	/**
	 * @brief Calculates contribution to Q from virtual loss
	 * 1 / (visits + vloss) is approximated by:
	 * 1 / visits - vloss / visits^2	if vloss < visits
	 * 1 / visits - visits / vloss^2	if vloss >= visits
	 */
	float getVloss(const ag::Edge *edge) noexcept
	{
		if (edge->getVirtualLoss() < edge->getVisits())
			return edge->getVirtualLoss() / ag::square(static_cast<float>(edge->getVisits()));
		else
			return edge->getVisits() / ag::square(static_cast<float>(edge->getVirtualLoss()));
	}
	template<typename T>
	float getQ(const T *n, float sf) noexcept
	{
		assert(n != nullptr);
		return n->getWinRate() + sf * n->getDrawRate();
	}
	template<typename T>
	float getProvenQ(const T *n) noexcept
	{
		assert(n != nullptr);
		return static_cast<int>(n->getProvenValue() == ag::ProvenValue::WIN) - static_cast<int>(n->getProvenValue() == ag::ProvenValue::LOSS);
	}
	template<typename T>
	float getBalance(const T *n) noexcept
	{
		assert(n != nullptr);
		return -fabsf(n->getWinRate() - (1.0f - n->getWinRate() - n->getDrawRate()));
	}
}

namespace ag
{
	PuctSelector::PuctSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	PuctSelector* PuctSelector::clone() const
	{
		return new PuctSelector(exploration_constant, style_factor);
	}
	Edge* PuctSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const float parent_value = 1.0f - getQ(node, style_factor);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());

		Edge *selected = node->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q = parent_value;
			if (edge->getVisits() > 0)
				Q = getQ(edge, style_factor) - getVloss(edge) - 10.0f * edge->isProven();
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

	UctSelector::UctSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	UctSelector* UctSelector::clone() const
	{
		return new UctSelector(exploration_constant, style_factor);
	}
	Edge* UctSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const float parent_value = 1.0f - getQ(node, style_factor);
		const float log_visit = logf(node->getVisits());

		Edge *selected = node->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q = parent_value;
			if (edge->getVisits() > 0)
				Q = getQ(edge, style_factor) - getVloss(edge) - 10.0f * edge->isProven();
			float U = exploration_constant * sqrtf(log_visit / (1.0f + edge->getVisits()));
			float P = edge->getPolicyPrior() / (1.0f + edge->getVisits());

			if (Q + U + P > bestValue)
			{
				selected = edge;
				bestValue = Q + U + P;
			}
		}
		assert(selected != node->end()); // this must never happen
		return selected;
	}

	BalancedSelector::BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector) :
			balance_depth(balanceDepth),
			base_selector(baseSelector.clone())
	{
	}
	BalancedSelector* BalancedSelector::clone() const
	{
		return new BalancedSelector(balance_depth, *base_selector);
	}
	Edge* BalancedSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->getDepth() < balance_depth)
		{
			Edge *selected = node->end();
			float bestValue = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
			{
				float Q = getBalance(edge) - getVloss(edge);
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
			return base_selector->select(node);
	}

	ValueSelector::ValueSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	ValueSelector* ValueSelector::clone() const
	{
		return new ValueSelector(style_factor);
	}
	Edge* ValueSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = node->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q = getQ(edge, style_factor) + 1.0e6f * getProvenQ(edge);
			if (Q > bestValue)
			{
				selected = edge;
				bestValue = Q;
			}
		}
		assert(selected != node->end()); // this must never happen
		return selected;
	}

	VisitSelector* VisitSelector::clone() const
	{
		return new VisitSelector();
	}
	Edge* VisitSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = node->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if (edge->getVisits() > bestValue)
			{
				selected = edge;
				bestValue = edge->getVisits();
			}
		}
		assert(selected != node->end()); // this must never happen
		return selected;
	}

	BestEdgeSelector::BestEdgeSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	BestEdgeSelector* BestEdgeSelector::clone() const
	{
		return new BestEdgeSelector(style_factor);
	}
	Edge* BestEdgeSelector::select(const Node *node) const noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = node->end();
		double bestValue = std::numeric_limits<double>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			double value = edge->getVisits() + (edge->getValue().win + style_factor * edge->getValue().draw) * node->getVisits()
					+ 0.001 * edge->getPolicyPrior() + getProvenQ(edge) * 1.0e9;
			if (value > bestValue)
			{
				selected = edge;
				bestValue = value;
			}
		}
		assert(selected != node->end()); // this must never happen
		return selected;
	}

} /* namespace ag */

