/*
 * EdgeSelector.cpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <cassert>

namespace
{
	using namespace ag;

	float getVirtualLoss(const Edge &edge) noexcept
	{
		const float visits = 1.0e-8f + edge.getVisits();
		const float virtual_loss = edge.getVirtualLoss();
		return visits / (visits + virtual_loss);
	}

	struct UCT
	{
			const float parent_log_visit;
			const float exploration_constant;
			const float style_factor;
			UCT(int parentVisits, float explorationConstant, float styleFactor) noexcept :
					parent_log_visit(logf(parentVisits)),
					exploration_constant(explorationConstant),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = edge.getExpectation(style_factor) * getVirtualLoss(edge);
				const float U = exploration_constant * sqrtf(parent_log_visit / (1.0f + edge.getVisits()));
				const float P = edge.getPolicyPrior() / (1.0f + edge.getVisits());
				return Q + U + P;
			}
	};
	struct PUCT
	{
			const float parent_cbrt_visit;
			const float style_factor;
			PUCT(const Node &parent, float explorationConstant, float styleFactor) noexcept :
					parent_cbrt_visit(explorationConstant * sqrtf(parent.getVisits())),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = edge.getExpectation(style_factor) * getVirtualLoss(edge);
				const float U = edge.getPolicyPrior() * parent_cbrt_visit / (1.0f + edge.getVisits());
				return Q + U;
			}
	};
	struct PUCT_parent
	{
			const float parent_sqrt_visit;
			const float parent_q;
			const float style_factor;
			PUCT_parent(const Node &parent, float explorationConstant, float styleFactor) noexcept :
					parent_sqrt_visit(explorationConstant * sqrtf(parent.getVisits())),
					parent_q(parent.getExpectation(styleFactor)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : parent_q;
				const float U = edge.getPolicyPrior() * parent_sqrt_visit / (1.0f + edge.getVisits());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct MaxValue
	{
			const float style_factor;
			MaxValue(float styleFactor) noexcept :
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				switch (edge.getScore().getProvenValue())
				{
					case ProvenValue::LOSS:
						return -100.0f + 0.1f * edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						return edge.getExpectation(style_factor);
					case ProvenValue::WIN:
						return +101.0f - 0.1f * edge.getScore().getDistance();
				}
			}
	};
	struct BestEdge
	{
			const int parent_visits;
			const float style_factor;
			BestEdge(int parentVisits, float styleFactor) noexcept :
					parent_visits(parentVisits),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				return edge.getVisits() + edge.getExpectation(style_factor) * parent_visits + 0.001f * edge.getPolicyPrior();
			}
	};
	struct MaxBalance
	{
			float operator()(const Edge &edge) const noexcept
			{
				switch (edge.getScore().getProvenValue())
				{
					case ProvenValue::LOSS:
						return 0.0f;
					case ProvenValue::DRAW:
						return 1.0f * getVirtualLoss(edge);
					default:
					case ProvenValue::UNKNOWN:
						return (1.0f - std::fabs(edge.getWinRate() - edge.getLossRate())) * getVirtualLoss(edge);
					case ProvenValue::WIN:
						return 0.0f;
				}
			}
	};

}

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
	Edge* PUCTSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const PUCT op(*node, exploration_constant, style_factor);
		const EdgeComparator<PUCT> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

	PUCTSelector_parent::PUCTSelector_parent(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> PUCTSelector_parent::clone() const
	{
		return std::make_unique<PUCTSelector_parent>(exploration_constant, style_factor);
	}
	Edge* PUCTSelector_parent::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const PUCT_parent op(*node, exploration_constant, style_factor);
		const EdgeComparator<PUCT_parent> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

	UCTSelector::UCTSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> UCTSelector::clone() const
	{
		return std::make_unique<UCTSelector>(exploration_constant, style_factor);
	}
	Edge* UCTSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const UCT op(node->getVisits(), exploration_constant, style_factor);
		const EdgeComparator<UCT> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

	BalancedSelector::BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector) :
			balance_depth(balanceDepth),
			base_selector(baseSelector.clone())
	{
	}
	std::unique_ptr<EdgeSelector> BalancedSelector::clone() const
	{
		return std::make_unique<BalancedSelector>(balance_depth, *base_selector);
	}
	Edge* BalancedSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->getDepth() < balance_depth)
		{
			const MaxBalance op;
			const EdgeComparator<MaxBalance> greater_than(op);

			Edge *best_edge = node->begin();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (greater_than(*edge, *best_edge))
					best_edge = edge;
			return best_edge;
		}
		else
			return base_selector->select(node);
	}

	MaxValueSelector::MaxValueSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> MaxValueSelector::clone() const
	{
		return std::make_unique<MaxValueSelector>(style_factor);
	}
	Edge* MaxValueSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const MaxValue op(style_factor);
		const EdgeComparator<MaxValue> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

	std::unique_ptr<EdgeSelector> MaxVisitSelector::clone() const
	{
		return std::make_unique<MaxVisitSelector>();
	}
	Edge* MaxVisitSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		Edge *selected = nullptr;
		int max_visits = std::numeric_limits<int>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if (edge->getVisits() > max_visits)
			{
				selected = edge;
				max_visits = edge->getVisits();
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

	BestEdgeSelector::BestEdgeSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> BestEdgeSelector::clone() const
	{
		return std::make_unique<BestEdgeSelector>(style_factor);
	}
	Edge* BestEdgeSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const BestEdge op(node->getVisits(), style_factor);
		const EdgeComparator<BestEdge> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

} /* namespace ag */
