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
	float scale_exploration(float base_exploration, const Node *parent) noexcept
	{
		return base_exploration + std::log(1.0f + parent->getVisits() / 20000.0f);
	}

	struct PUCT_parent_pow
	{
			const float denominator;
			const float parent_q;
			const float style_factor;
			PUCT_parent_pow(const Node *parent, float explorationConstant, float explorationExponent, float styleFactor) noexcept :
					denominator(explorationConstant * std::pow(parent->getVisits() + parent->getVirtualLoss(), explorationExponent)),
					parent_q(parent->getExpectation(styleFactor)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				return this->operator ()(edge, edge.getPolicyPrior());
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : parent_q;
				const float U = externalPrior * denominator / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U;
			}
	};

	struct UCT
	{
			const float parent_log_visit;
			const float parent_q;
			const float style_factor;
			UCT(const Node *parent, float styleFactor) noexcept :
					parent_log_visit(logf(parent->getVisits())),
					parent_q(parent->getExpectation(styleFactor)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = edge.getExpectation(style_factor) * getVirtualLoss(edge);
				const float P = edge.getPolicyPrior() / (1.0f + edge.getVisits());
				return Q + P;
			}
	};
	struct PUCT
	{
			const float parent_sqrt_visit;
			const float style_factor;
			PUCT(const Node *parent, float explorationConstant, float styleFactor) noexcept :
					parent_sqrt_visit(scale_exploration(explorationConstant, parent) * sqrtf(parent->getVisits() + parent->getVirtualLoss())),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				return this->operator ()(edge, edge.getPolicyPrior());
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = edge.getExpectation(style_factor);
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct PUCT_parent
	{
			const float parent_sqrt_visit;
			const float parent_q;
			const float style_factor;
			PUCT_parent(const Node *parent, float explorationConstant, float styleFactor) noexcept :
					parent_sqrt_visit(scale_exploration(explorationConstant, parent) * sqrtf(parent->getVisits() + parent->getVirtualLoss())),
					parent_q(parent->getExpectation(styleFactor)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				return this->operator ()(edge, edge.getPolicyPrior());
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : parent_q;
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
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
				switch (edge.getProvenValue())
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
	struct MaxPolicy
	{
			float operator()(const Edge &edge) const noexcept
			{
				return edge.getPolicyPrior();
			}
	};
	struct MaxVisit
	{
			float operator()(const Edge &edge) const noexcept
			{
				return edge.getVisits();
			}
	};
	struct MinVisit
	{
			float operator()(const Edge &edge) const noexcept
			{
				return -edge.getVisits();
			}
	};
	struct BestEdge
	{
			const int parent_visits;
			const float style_factor;
			BestEdge(const Node *parent, float styleFactor) noexcept :
					parent_visits(parent->getVisits()),
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

	template<class Op>
	Edge* find_best_edge(const Node *node, Op op) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		Edge *best_edge = nullptr;
		float best_value = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			const float value = op(*edge);
			if (value > best_value)
			{
				best_value = value;
				best_edge = edge;
			}
		}
		assert(best_edge != nullptr);
		return best_edge;
	}

	std::vector<float> createNoise(const Node *rootNode, float noiseWeight)
	{
		std::vector<float> result(rootNode->numberOfEdges());
		float sum = 0.0f;
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = pow(randFloat(), 4) * (1.0f - sum);
			sum += result[i];
		}
		std::random_shuffle(result.begin(), result.end());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = (1.0f - noiseWeight) * rootNode->getEdge(i).getPolicyPrior() + noiseWeight * result[i];
		return result;
	}
	std::vector<float> createDirichletNoise(const Node *rootNode, float noiseWeight)
	{
		std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
		std::gamma_distribution<double> noise(0.05);

		std::vector<float> result(rootNode->numberOfEdges());
		float sum = 0.0f;
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = noise(generator);
			sum += result[i];
		}
		std::random_shuffle(result.begin(), result.end());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = (1.0f - noiseWeight) * rootNode->getEdge(i).getPolicyPrior() + noiseWeight * result[i] / sum;
		return result;
	}
	std::vector<float> createGumbelNoise(const Node *rootNode, float noiseWeight)
	{
		float max_value = std::numeric_limits<float>::lowest();
		std::vector<float> result(rootNode->numberOfEdges());
		for (size_t i = 0; i < result.size(); i++)
		{
			const float gumbel_noise = -safe_log(-safe_log(randFloat()));
			result[i] = safe_log(rootNode->getEdge(i).getPolicyPrior()) + noiseWeight * gumbel_noise;
			max_value = std::max(max_value, result[i]);
		}

		float sum = 0.0f;
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = std::exp(result[i] - max_value);
			if (result[i] < 1.0e-9f)
				result[i] = 0.0f;
			sum += result[i];
		}

		assert(sum > 0.0f);
		sum = 1.0f / sum;
		for (size_t i = 0; i < result.size(); i++)
			result[i] *= sum;
		return result;
	}
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

		const PUCT op(node, exploration_constant, style_factor);

		Edge *best_edge = node->begin();
		float best_value = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if ((edge->isProven() or best_edge->isProven()) and edge->getScore() != best_edge->getScore())
			{
				if (edge->getScore() > best_edge->getScore())
					best_edge = edge;
			}
			else
			{
				const float value = op(*edge);
				if (value > best_value)
				{
					best_value = value;
					best_edge = edge;
				}
			}
		}
		return best_edge;
	}

	PUCTSelector_parent::PUCTSelector_parent(float exploration_c, float exploration_exp, float styleFactor) :
			exploration_constant(exploration_c),
			exploration_exponent(exploration_exp),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> PUCTSelector_parent::clone() const
	{
		return std::make_unique<PUCTSelector_parent>(exploration_constant, exploration_exponent, style_factor);
	}
	Edge* PUCTSelector_parent::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const PUCT_parent_pow op(node, exploration_constant, exploration_exponent, style_factor);

		Edge *best_edge = node->begin();
		float best_value = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if ((edge->isProven() or best_edge->isProven()) and edge->getScore() != best_edge->getScore())
			{
				if (edge->getScore() > best_edge->getScore())
					best_edge = edge;
			}
			else
			{
				const float value = op(*edge);
				if (value > best_value)
				{
					best_value = value;
					best_edge = edge;
				}
			}
		}
		return best_edge;
	}

	NoisyPUCTSelector::NoisyPUCTSelector(int rootDepth, float exploration, float styleFactor) :
			root_depth(rootDepth),
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> NoisyPUCTSelector::clone() const
	{
		return std::make_unique<NoisyPUCTSelector>(root_depth, exploration_constant, style_factor);
	}
	Edge* NoisyPUCTSelector::select(const Node *node) noexcept
	{
		if (node->getDepth() == root_depth)
		{
			if (not is_initialized)
			{
				noisy_policy = createNoise(node, 0.25f);
//				noisy_policy = createDirichletNoise(node, 0.25f);
//				noisy_policy = createGumbelNoise(node, 1.0f);
				is_initialized = true;
			}
			assert(node != nullptr);
			assert(node->isLeaf() == false);

			const PUCT_parent op(node, exploration_constant, style_factor);

			Edge *best_edge = node->begin();
			float best_value = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
			{
				if ((edge->isProven() or best_edge->isProven()) and edge->getScore() != best_edge->getScore())
				{
					if (edge->getScore() > best_edge->getScore())
						best_edge = edge;
				}
				else
				{
					const float noisy_prior = noisy_policy[std::distance(node->begin(), edge)];
					const float value = op(*edge, noisy_prior);
					if (value > best_value)
					{
						best_value = value;
						best_edge = edge;
					}
				}
			}

//			Edge *best_edge = nullptr;
//			float best_value = std::numeric_limits<float>::lowest();
//			for (Edge *edge = node->begin(); edge < node->end(); edge++)
//			{
//				const float noisy_prior = noisy_policy[std::distance(node->begin(), edge)];
//				const float value = op(*edge, noisy_prior) - 1.0e4f * static_cast<int>(edge->isProven());
//				if (value > best_value)
//				{
//					best_value = value;
//					best_edge = edge;
//				}
//			}
//			assert(best_edge != nullptr);
			return best_edge;
		}
		else
			return PUCTSelector_parent(exploration_constant, style_factor).select(node);
	}

	UCTSelector::UCTSelector(float styleFactor) :
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> UCTSelector::clone() const
	{
		return std::make_unique<UCTSelector>(style_factor);
	}
	Edge* UCTSelector::select(const Node *node) noexcept
	{
		const UCT op(node, style_factor);
		return find_best_edge(node, op);
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
		if (node->getDepth() < balance_depth)
			return find_best_edge(node, MaxBalance());
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
		const MaxValue op(style_factor);
		return find_best_edge(node, op);
	}

	std::unique_ptr<EdgeSelector> MaxPolicySelector::clone() const
	{
		return std::make_unique<MaxPolicySelector>();
	}
	Edge* MaxPolicySelector::select(const Node *node) noexcept
	{
		return find_best_edge(node, MaxPolicy());
	}

	std::unique_ptr<EdgeSelector> MaxVisitSelector::clone() const
	{
		return std::make_unique<MaxVisitSelector>();
	}
	Edge* MaxVisitSelector::select(const Node *node) noexcept
	{
		return find_best_edge(node, MaxVisit());
	}

	std::unique_ptr<EdgeSelector> MinVisitSelector::clone() const
	{
		return std::make_unique<MinVisitSelector>();
	}
	Edge* MinVisitSelector::select(const Node *node) noexcept
	{
		return find_best_edge(node, MinVisit());
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
		const BestEdge op(node, style_factor);
		const EdgeComparator<BestEdge> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(*edge, *best_edge))
				best_edge = edge;
		return best_edge;
	}

} /* namespace ag */
