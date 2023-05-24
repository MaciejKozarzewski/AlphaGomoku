/*
 * EdgeSelector.cpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/random.hpp>
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

	struct PUCT_q_head
	{
			const float parent_sqrt_visit;
			const float style_factor;
			PUCT_q_head(const Node *parent, float explorationConstant, float explorationExponent, float styleFactor) noexcept :
					parent_sqrt_visit(explorationConstant * std::pow(parent->getVisits() + parent->getVirtualLoss(), explorationExponent)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = edge.getExpectation(style_factor);
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct PUCT
	{
			const float parent_sqrt_visit;
			const float style_factor;
			const float initial_Q;
			PUCT(const Node *parent, float explorationConstant, float explorationExponent, float styleFactor, float initialQ) noexcept :
					parent_sqrt_visit(explorationConstant * std::pow(parent->getVisits() + parent->getVirtualLoss(), explorationExponent)),
					style_factor(styleFactor),
					initial_Q(initialQ)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : initial_Q;
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct UCT
	{
			const float exploration_constant;
			const float parent_log_visit;
			const float style_factor;
			const float initial_Q;
			UCT(const Node *parent, float explorationConstant, float styleFactor, float initialQ) noexcept :
					exploration_constant(explorationConstant),
					parent_log_visit(std::log(parent->getVisits() + parent->getVirtualLoss())),
					style_factor(styleFactor),
					initial_Q(initialQ)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : initial_Q;
				const float U = exploration_constant * std::sqrt(parent_log_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss()));
				const float P = externalPrior / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U + P;
			}
	};
	struct LCB
	{
			const float exploration_constant;
			const float parent_log_visit;
			const float style_factor;
			const float initial_Q;
			LCB(const Node *parent, float explorationConstant, float styleFactor) noexcept :
					exploration_constant(explorationConstant),
					parent_log_visit(std::log(parent->getVisits() + parent->getVirtualLoss())),
					style_factor(styleFactor),
					initial_Q(parent->getExpectation(styleFactor))
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1.0e6f + edge.getScore().getDistance() + externalPrior;
					case ProvenValue::DRAW:
					case ProvenValue::UNKNOWN:
					default:
					{
						const float Q = (edge.getVisits() > 0) ? edge.getExpectation(style_factor) : initial_Q;
						const float U = exploration_constant * std::sqrt(parent_log_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss()));
						return Q * getVirtualLoss(edge) - U;
					}
					case ProvenValue::WIN:
						return +1.0e6f - edge.getScore().getDistance() + externalPrior;
				}
			}
	};
	struct MaxValue
	{
			const float style_factor;
			MaxValue(float styleFactor) noexcept :
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge, float) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1000.0f + edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						return edge.getExpectation(style_factor);
					case ProvenValue::WIN:
						return +1000.0f - edge.getScore().getDistance();
				}
			}
	};
	struct MaxPolicy
	{
			float operator()(const Edge &edge, float) const noexcept
			{
				return edge.getPolicyPrior();
			}
	};
	struct MaxVisit
	{
			float operator()(const Edge &edge, float) const noexcept
			{
				return edge.getVisits();
			}
	};
	struct MinVisit
	{
			float operator()(const Edge &edge, float) const noexcept
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
			float operator()(const Edge &edge, float) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1.0e8f + edge.getScore().getDistance();
					default:
					case ProvenValue::DRAW:
					case ProvenValue::UNKNOWN:
						return edge.getVisits() + edge.getExpectation(style_factor) * parent_visits + 0.001f * edge.getPolicyPrior();
					case ProvenValue::WIN:
						return +1.0e8f - edge.getScore().getDistance();
				}
			}
	};
	struct MaxBalance
	{
			float operator()(const Edge &edge, float) const noexcept
			{
				switch (edge.getProvenValue())
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

	template<class Op, bool UseNoise>
	Edge* find_best_edge_impl(const Node *node, const Op &op, const std::vector<float> &noise) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		Edge *best_edge = nullptr;
		float best_value = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			const float policy_prior = UseNoise ? noise[std::distance(node->begin(), edge)] : edge->getPolicyPrior();
			const float value = op(*edge, policy_prior);
			if (value > best_value)
			{
				best_value = value;
				best_edge = edge;
			}
		}
		assert(best_edge != nullptr);
		return best_edge;
	}

	template<class Op>
	Edge* find_best_edge(const Node *node, Op op, const std::vector<float> &noise) noexcept
	{
		if (noise.empty())
			return find_best_edge_impl<Op, false>(node, op, std::vector<float>());
		else
			return find_best_edge_impl<Op, true>(node, op, noise);
	}
	template<class Op>
	Edge* find_best_edge(const Node *node, Op op) noexcept
	{
		return find_best_edge_impl<Op, false>(node, op, std::vector<float>());
	}

	std::vector<float> applyCustomNoise(const Node *rootNode, float noiseWeight)
	{
		std::vector<float> result = createCustomNoise(rootNode->numberOfEdges());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = (1.0f - noiseWeight) * rootNode->getEdge(i).getPolicyPrior() + noiseWeight * result[i];
		return result;
	}
	std::vector<float> applyDirichletNoise(const Node *rootNode, float noiseWeight)
	{
		std::vector<float> result = createDirichletNoise(rootNode->numberOfEdges(), 0.05f);
		for (size_t i = 0; i < result.size(); i++)
			result[i] = (1.0f - noiseWeight) * rootNode->getEdge(i).getPolicyPrior() + noiseWeight * result[i];
		return result;
	}
	std::vector<float> applyGumbelNoise(const Node *rootNode, float noiseWeight)
	{
		std::vector<float> result = createGumbelNoise(rootNode->numberOfEdges());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = safe_log(rootNode->getEdge(i).getPolicyPrior()) + noiseWeight * result[i];
		softmax(result);
		return result;
	}
}

namespace ag
{
	std::unique_ptr<EdgeSelector> EdgeSelector::create(const EdgeSelectorConfig &config)
	{
		if (config.policy == "puct")
			return std::make_unique<PUCTSelector>(config);
		if (config.policy == "max_value")
			return std::make_unique<MaxValueSelector>(config);
		if (config.policy == "max_policy")
			return std::make_unique<MaxPolicySelector>();
		if (config.policy == "max_visit")
			return std::make_unique<MaxVisitSelector>();
		if (config.policy == "min_visit")
			return std::make_unique<MinVisitSelector>();
		if (config.policy == "best")
			return std::make_unique<BestEdgeSelector>(config);
		if (config.policy == "lcb")
			return std::make_unique<LCBSelector>(config);
		throw std::logic_error("Unknown selection policy '" + config.policy + "'");
	}

	PUCTSelector::PUCTSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant),
			exploration_exponent(config.exploration_exponent),
			style_factor(config.style_factor)
	{
	}
	std::unique_ptr<EdgeSelector> PUCTSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "puct";
		config.init_to = init_to;
		config.noise_type = noise_type;
		config.noise_weight = noise_weight;
		config.exploration_constant = exploration_constant;
		config.exploration_exponent = exploration_exponent;
		config.style_factor = style_factor;
		return std::make_unique<PUCTSelector>(config);
	}
	Edge* PUCTSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const bool use_noise = node->isRoot() and noise_weight > 0.0f;
		if (use_noise and noisy_policy.empty())
		{ // initialize noise
			if (noise_type == "custom")
				noisy_policy = applyCustomNoise(node, noise_weight);
			if (noise_type == "dirichlet")
				noisy_policy = applyDirichletNoise(node, noise_weight);
			if (noise_type == "gumbel")
				noisy_policy = applyGumbelNoise(node, noise_weight);
		}

		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());

		if (init_to == "q_head")
		{
			const PUCT_q_head op(node, c_puct, exploration_exponent, style_factor);
			if (use_noise)
				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
		}
		else
		{
			float initial_q = 0.0f; // the default is 'init to loss'
			if (init_to == "parent")
				initial_q = node->getValue().getExpectation(style_factor);
			else
			{
				if (init_to == "draw")
					initial_q = 0.5;
			}
			const PUCT op(node, c_puct, exploration_exponent, style_factor, initial_q);
			if (use_noise)
				return find_best_edge_impl<PUCT, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
		}
	}

	LCBSelector::LCBSelector(const EdgeSelectorConfig &config) :
			exploration_constant(config.exploration_constant),
			style_factor(config.style_factor)
	{
	}
	std::unique_ptr<EdgeSelector> LCBSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "lcb";
		config.exploration_constant = exploration_constant;
		config.style_factor = style_factor;
		return std::make_unique<LCBSelector>(config);
	}
	Edge* LCBSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const LCB op(node, exploration_constant, style_factor);
		return find_best_edge<LCB>(node, op);
	}

	BalancedSelector::BalancedSelector() :
			balance_depth(std::numeric_limits<int>::max()),
			base_selector(nullptr)
	{
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
		{
			assert(base_selector != nullptr);
			return base_selector->select(node);
		}
	}

	MaxValueSelector::MaxValueSelector(float styleFactor) noexcept :
			style_factor(styleFactor)
	{
	}
	MaxValueSelector::MaxValueSelector(const EdgeSelectorConfig &config) :
			style_factor(config.style_factor)
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
	BestEdgeSelector::BestEdgeSelector(const EdgeSelectorConfig &config) :
			style_factor(config.style_factor)
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
		return find_best_edge(node, op);
	}

} /* namespace ag */
