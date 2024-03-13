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

	float inv_erf_naive(float P, float precision = 1.0e-3f) noexcept
	{
		static constexpr float table[14] = { 0.0f, 0.191462461274013f, 0.341344746068543f, 0.433192798731142f, 0.477249868051821f, 0.493790334674224f,
				0.49865010196837f, 0.499767370920964f, 0.499968328758167f, 0.499996602326875f, 0.499999713348428f, 0.499999981010437f,
				0.499999999013412f, 0.5f };

//		const float sign = (P > 0.5f) ? +1.0f : -1.0f;
//		const float tmp = std::abs(P - 0.5f);
		float x = 0.0f;
//		for (int i = 1; i < 14; i++)
//			if (tmp <= table[i])
//			{
//				x = sign * 0.5f * ((i - 1) + (tmp - table[i - 1]) / (table[i] - table[i - 1]));
//				break;
//			}

		for (int i = 0; i < 1000; i++)
		{
			const float f = std::erf(x) - P;
			const float df = 2.0f / std::sqrt(3.141592653589) * std::exp(-x * x);

			const float new_x = x - clip(f / df, -1.0f, 1.0f);
			const float diff = std::abs(new_x - x);
//			std::cout << "mean=" << mean << ", f=" << f << ", df=" << df << ", new mean=" << new_mean << " (" << diff << ")\n";

			if (diff <= precision)
				break;
			x = new_x;
		}
		return x;
	}
	float inv_erf(float x) noexcept
	{
		if (x == 1.0f)
			return +100.0f;
		if (x == -1.0f)
			return -100.0f;

		float p, r, t;
		t = fmaf(x, 0.0f - x, 1.0f);
		t = std::log(t);
		if (fabsf(t) > 6.125f)
		{ // maximum ulp error = 2.35793
			p = 3.03697567e-10f; //  0x1.4deb44p-32
			p = fmaf(p, t, 2.93243101e-8f); //  0x1.f7c9aep-26
			p = fmaf(p, t, 1.22150334e-6f); //  0x1.47e512p-20
			p = fmaf(p, t, 2.84108955e-5f); //  0x1.dca7dep-16
			p = fmaf(p, t, 3.93552968e-4f); //  0x1.9cab92p-12
			p = fmaf(p, t, 3.02698812e-3f); //  0x1.8cc0dep-9
			p = fmaf(p, t, 4.83185798e-3f); //  0x1.3ca920p-8
			p = fmaf(p, t, -2.64646143e-1f); // -0x1.0eff66p-2
			p = fmaf(p, t, 8.40016484e-1f); //  0x1.ae16a4p-1
		}
		else
		{ // maximum ulp error = 2.35002
			p = 5.43877832e-9f; //  0x1.75c000p-28
			p = fmaf(p, t, 1.43285448e-7f); //  0x1.33b402p-23
			p = fmaf(p, t, 1.22774793e-6f); //  0x1.499232p-20
			p = fmaf(p, t, 1.12963626e-7f); //  0x1.e52cd2p-24
			p = fmaf(p, t, -5.61530760e-5f); // -0x1.d70bd0p-15
			p = fmaf(p, t, -1.47697632e-4f); // -0x1.35be90p-13
			p = fmaf(p, t, 2.31468678e-3f); //  0x1.2f6400p-9
			p = fmaf(p, t, 1.15392581e-2f); //  0x1.7a1e50p-7
			p = fmaf(p, t, -2.32015476e-1f); // -0x1.db2aeep-3
			p = fmaf(p, t, 8.86226892e-1f); //  0x1.c5bf88p-1
		}
		r = x * p;
		return r;

	}

	struct ThompsonSampling
	{
			const float parent_sqrt_visit;
			const float parent_mean;
			const float parent_variance;
			ThompsonSampling(const Node *parent, float explorationConstant, float parentQ) noexcept :
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss())),
					parent_mean(parentQ),
					parent_variance(0.2f)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = edge.sampleFromDistribution();
				const float U = 10.0f * externalPrior / (1.0f + edge.getVisits() + edge.getVirtualLoss());
//				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
//				const float U = 0.0f;
				return Q + U;
			}
	};
	struct ThompsonSamplingNormal
	{
			static constexpr float prior_variance = 0.6f;

			const float parent_sqrt_visit;
			float best_mean = 0.0f;
			float best_variance = prior_variance;

			ThompsonSamplingNormal(const Node *parent, float explorationConstant) noexcept :
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss()))
			{
				for (auto edge = parent->begin(); edge < parent->end(); edge++)
					if (edge->getExpectation() > best_mean)
					{
						best_mean = edge->getExpectation();
						if (edge->getVisits() >= 2)
							best_variance = edge->getVariance();
					}
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
//				std::cout << "Calculating data for " << edge.toString() << '\n';
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1000.0f + edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						return +1000.0f - edge.getScore().getDistance();
				}

				const int N = edge.getVisits();
				const float mean = (N >= 1) ? (edge.getExpectation() * getVirtualLoss(edge)) : fit_mean(edge.getPolicyPrior());
				const float variance = (N >= 2) ? edge.getVariance() : prior_variance;
//				std::cout << "using mean = " << mean << ", variance = " << variance << '\n';

				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return randGaussian(mean + U, variance);
			}
		private:
			float get_dist_estimate(float P) const noexcept
			{
				static constexpr float table[14] = { 0.0f, 0.191462461274013f, 0.341344746068543f, 0.433192798731142f, 0.477249868051821f,
						0.493790334674224f, 0.49865010196837f, 0.499767370920964f, 0.499968328758167f, 0.499996602326875f, 0.499999713348428f,
						0.499999981010437f, 0.499999999013412f, 0.5f };

				const float sign = (P > 0.5f) ? +1.0f : -1.0f;
				const float x = std::abs(P - 0.5f);
				for (int i = 1; i < 14; i++)
					if (x <= table[i])
					{
						const float tmp = (x - table[i - 1]) / (table[i] - table[i - 1]);
						return sign * 0.5f * ((i - 1) + tmp);
					}
				return 0.0f;
			}
			float fit_mean(float P) const noexcept
			{
				return best_mean + std::sqrt(2.0f * (best_variance + prior_variance)) * inv_erf(2.0f * P - 1.0f);

////				std::cout << "best mean=" << best_mean << ", bestvar=" << best_var << ", own var=" << own_var << ", target P=" << P << '\n';
//				const float tmp = 1.0f / std::sqrt(2.0f * (best_var + own_var));
//
//				const float joint_stddev = std::sqrt(best_var + own_var);
//				float mean = best_mean + joint_stddev * get_dist_estimate(P);
//				for (int i = 0; i < 10; i++)
//				{
//					const float x = (best_mean - mean) * tmp;
//					const float f = 0.5f * (1.0f - std::erf(x)) - P;
//					const float df = 1.0f / std::sqrt(3.141592653589) * std::exp(-x * x);
//
//					const float new_mean = mean - clip(f / df, -1.0f, 1.0f);
//					const float diff = std::abs(new_mean - mean);
////					std::cout << "mean=" << mean << ", f=" << f << ", df=" << df << ", new mean=" << new_mean << " (" << diff << ")\n";
//
//					if (diff <= precision)
//						break;
//					mean = new_mean;
//				}
//				return mean;
			}
	};
	struct KLUCB
	{
			const float parent_visits;
			KLUCB(const Node *parent, float explorationConstant) noexcept :
					parent_visits(parent->getVisits() + parent->getVirtualLoss())
			{
//				std::cout << "parent = " << parent->toString() << '\n';
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1000.0f + edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						return +1000.0f - edge.getScore().getDistance();
				}
				if (edge.getVisits() == 0)
				{
					const float P = edge.getPolicyPrior();
					return (randFloat() <= P) ? (100.0f + P) : P;
				}

//				std::cout << "edge = " << edge.toString() << '\n';
				const float T = std::log(parent_visits) / (edge.getVisits() + edge.getVirtualLoss());
				const float Q = fit_kl(edge.getExpectation() * getVirtualLoss(edge), T);
				const float U = externalPrior / (1.0f + edge.getVisits() + edge.getVirtualLoss());
//				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
//				const float U = 0.0f;
//				if (edge.getVisits() == 5)
//					exit(-1);
				return Q + U;
			}
		private:
			float fit_kl(float p, float T, float precision = 1.0e-3f) const noexcept
			{
				const float rhs = p * safe_log(p) + (1.0f - p) * safe_log(1.0f - p) - T;
//				std::cout << "p = " << p << ", T = " << T << '\n';

				float q = 0.5f * (1.0f + p); // initial guess for q
				for (int i = 0; i < 100; i++)
				{
					const float f = p * safe_log(q) + (1.0f - p) * safe_log(1.0f - q) - rhs;
					const float df = (p - q) / (q * (1 - q));
					const float step_size = 0.9f * (1.0f - q);

					const float new_q = q - std::max(-step_size, f / df);
					const float diff = std::abs(new_q - q);
//					std::cout << "q = " << q << ", f = " << f << ", df = " << df << ", step = " << step_size << ", new q = " << new_q << " (" << diff
//							<< ")\n";

					if (diff <= precision)
						break;
					q = new_q;
				}
				return q;
			}
	};
	struct BayesUCB
	{
			static constexpr float prior_variance = 10.0f;

			const float parent_visits;
			const float parent_sqrt_visit;
			float best_mean = 0.0f;
			float best_variance = prior_variance;

			BayesUCB(const Node *parent, float explorationConstant) noexcept :
					parent_visits(parent->getVisits()),
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss()))
			{
				for (auto edge = parent->begin(); edge < parent->end(); edge++)
					if (edge->getExpectation() > best_mean)
					{
						best_mean = edge->getExpectation();
						if (edge->getVisits() >= 2)
							best_variance = edge->getVariance();
					}
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1000.0f + edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						return +1000.0f - edge.getScore().getDistance();
				}
				const int N = edge.getVisits();
				const float mean = (N >= 1) ? (edge.getExpectation() * getVirtualLoss(edge)) : fit_mean(edge.getPolicyPrior());
				const float variance = (N >= 2) ? edge.getVariance() : prior_variance;
//				std::cout << "using mean = " << mean << ", variance = " << variance << '\n';

				const float U = 0.0f; //externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());

//				std::cout << "edge = " << edge.toString() << '\n';
				const float Q = get_quantile(1.0f - 1.0f / (1.0f + parent_visits), mean, variance);
//				std::cout << "Q = " << Q << '\n';
				return Q + U;
			}
		private:
			float fit_mean(float P) const noexcept
			{
				return get_quantile(P, best_mean, best_variance + prior_variance);
			}
			float get_quantile(float Q, float mean, float variance) const noexcept
			{
				return mean + std::sqrt(2.0f * variance) * inv_erf(2.0f * Q - 1.0f);
			}
	};
	struct PUCT_q_head
	{
			const float parent_sqrt_visit;
			PUCT_q_head(const Node *parent, float explorationConstant) noexcept :
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss()))
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = edge.getExpectation();
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct PUCT
	{
			const float parent_sqrt_visit;
			const float initial_Q;
			PUCT(const Node *parent, float explorationConstant, float initialQ) noexcept :
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss())),
					initial_Q(initialQ)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				switch (edge.getProvenValue())
				{
					case ProvenValue::LOSS:
						return -1000.0f + edge.getScore().getDistance();
					case ProvenValue::DRAW:
						return Value::draw().getExpectation();
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						return +1000.0f - edge.getScore().getDistance();
				}

				float Q = initial_Q;
				if (edge.isBeingExpanded())
					Q = -1000.0f;
				else
				{
					if (edge.getVisits() > 0)
						Q = edge.getExpectation() * getVirtualLoss(edge);
				}
				const float U = externalPrior * parent_sqrt_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q + U;
			}
	};
	struct UCB
	{
			const float parent_sqrt_visit;
			const float exploration_constant;
			const float parent_log_visit;
			const float initial_Q;
			UCB(const Node *parent, float explorationConstant, float initialQ) noexcept :
					parent_sqrt_visit(explorationConstant * std::sqrt(parent->getVisits() + parent->getVirtualLoss())),
					exploration_constant(explorationConstant),
					parent_log_visit(std::log(parent->getVisits() + parent->getVirtualLoss())),
					initial_Q(initialQ)
			{
			}
			float operator()(const Edge &edge, float externalPrior) const noexcept
			{
				const float Q = (edge.getVisits() > 0) ? edge.getExpectation() : initial_Q;
				const float U = exploration_constant * std::sqrt(parent_log_visit / (1.0f + edge.getVisits() + edge.getVirtualLoss()));
				const float P = externalPrior / (1.0f + edge.getVisits() + edge.getVirtualLoss());
				return Q * getVirtualLoss(edge) + U + P;
			}
	};
	struct LCB
	{
			const float exploration_constant;
			const float parent_log_visit;
			const float initial_Q;
			LCB(const Node *parent, float explorationConstant) noexcept :
					exploration_constant(explorationConstant),
					parent_log_visit(std::log(parent->getVisits() + parent->getVirtualLoss())),
					initial_Q(parent->getExpectation())
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
						const float Q = (edge.getVisits() > 0) ? edge.getExpectation() : initial_Q;
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
						return edge.getExpectation();
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
			BestEdge(const Node *parent) noexcept :
					parent_visits(parent->getVisits())
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
						return edge.getVisits() + edge.getExpectation() * parent_visits + 0.001f * edge.getPolicyPrior();
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

//		std::cout << "\n\n\n";
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
//		if (node->getVisits() == 100)
//			exit(-1);
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
		if (config.policy == "thompson")
			return std::make_unique<ThompsonSelector>(config);
		if (config.policy == "bayes_ucb")
			return std::make_unique<BayesUCBSelector>(config);
		if (config.policy == "kl_ucb")
			return std::make_unique<KLUCBSelector>(config);
		if (config.policy == "puct")
			return std::make_unique<PUCTSelector>(config);
		if (config.policy == "puct_fpu")
			return std::make_unique<PUCTfpuSelector>(config);
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
		if (config.policy == "ucb")
			return std::make_unique<UCBSelector>(config);
		if (config.policy == "lcb")
			return std::make_unique<LCBSelector>(config);
		throw std::logic_error("Unknown selection policy '" + config.policy + "'");
	}

	ThompsonSelector::ThompsonSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> ThompsonSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "thompson";
		config.init_to = init_to;
		config.noise_type = noise_type;
		config.noise_weight = noise_weight;
		config.exploration_constant = exploration_constant;
		return std::make_unique<ThompsonSelector>(config);
	}
	Edge* ThompsonSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		if (not node->isRoot())
		{
			const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
			float initial_q = node->getValue().getExpectation();
			const PUCT op(node, c_puct, initial_q);
			return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
		}

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

//		const ThompsonSampling op(node, c_puct, node->getExpectation());
//		if (use_noise)
//			return find_best_edge_impl<ThompsonSampling, true>(node, op, noisy_policy);
//		else
//			return find_best_edge_impl<ThompsonSampling, false>(node, op, noisy_policy);

		const ThompsonSamplingNormal op(node, c_puct);
		if (use_noise)
			return find_best_edge_impl<ThompsonSamplingNormal, true>(node, op, noisy_policy);
		else
			return find_best_edge_impl<ThompsonSamplingNormal, false>(node, op, noisy_policy);

//		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//		const float c_puct = exploration_constant;

//		if (init_to == "q_head")
//		{
//			const PUCT_q_head op(node, c_puct, exploration_exponent);
//			if (use_noise)
//				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
//		}
//		else
//		{
//			float initial_q = 0.0f; // the default is 'init to loss'
//			if (init_to == "parent")
//				initial_q = node->getValue().getExpectation();
//			else
//			{
//				if (init_to == "draw")
//					initial_q = 0.5f;
//				else
//					initial_q = 0.0f;
//			}
//			const UCB op(node, c_puct, exploration_exponent, initial_q);
//			if (use_noise)
//				return find_best_edge_impl<UCB, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<UCB, false>(node, op, noisy_policy);
//		}
	}

	KLUCBSelector::KLUCBSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> KLUCBSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "thompson";
		config.init_to = init_to;
		config.noise_type = noise_type;
		config.noise_weight = noise_weight;
		config.exploration_constant = exploration_constant;
		return std::make_unique<KLUCBSelector>(config);
	}
	Edge* KLUCBSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

//		if (not node->isRoot())
//		{
//			const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//			float initial_q = node->getValue().getExpectation();
//			const PUCT op(node, c_puct, initial_q);
//			return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
//		}

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
		const KLUCB op(node, c_puct);
		if (use_noise)
			return find_best_edge_impl<KLUCB, true>(node, op, noisy_policy);
		else
			return find_best_edge_impl<KLUCB, false>(node, op, noisy_policy);

//		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//		const float c_puct = exploration_constant;

//		if (init_to == "q_head")
//		{
//			const PUCT_q_head op(node, c_puct, exploration_exponent);
//			if (use_noise)
//				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
//		}
//		else
//		{
//			float initial_q = 0.0f; // the default is 'init to loss'
//			if (init_to == "parent")
//				initial_q = node->getValue().getExpectation();
//			else
//			{
//				if (init_to == "draw")
//					initial_q = 0.5f;
//				else
//					initial_q = 0.0f;
//			}
//			const UCB op(node, c_puct, exploration_exponent, initial_q);
//			if (use_noise)
//				return find_best_edge_impl<UCB, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<UCB, false>(node, op, noisy_policy);
//		}
	}

	BayesUCBSelector::BayesUCBSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> BayesUCBSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "thompson";
		config.init_to = init_to;
		config.noise_type = noise_type;
		config.noise_weight = noise_weight;
		config.exploration_constant = exploration_constant;
		return std::make_unique<BayesUCBSelector>(config);
	}
	Edge* BayesUCBSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

//		if (not node->isRoot())
//		{
//			const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//			float initial_q = node->getValue().getExpectation();
//			const PUCT op(node, c_puct, initial_q);
//			return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
//		}

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
		const BayesUCB op(node, c_puct);
		if (use_noise)
			return find_best_edge_impl<BayesUCB, true>(node, op, noisy_policy);
		else
			return find_best_edge_impl<BayesUCB, false>(node, op, noisy_policy);

//		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//		const float c_puct = exploration_constant;

//		if (init_to == "q_head")
//		{
//			const PUCT_q_head op(node, c_puct, exploration_exponent);
//			if (use_noise)
//				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
//		}
//		else
//		{
//			float initial_q = 0.0f; // the default is 'init to loss'
//			if (init_to == "parent")
//				initial_q = node->getValue().getExpectation();
//			else
//			{
//				if (init_to == "draw")
//					initial_q = 0.5f;
//				else
//					initial_q = 0.0f;
//			}
//			const UCB op(node, c_puct, exploration_exponent, initial_q);
//			if (use_noise)
//				return find_best_edge_impl<UCB, true>(node, op, noisy_policy);
//			else
//				return find_best_edge_impl<UCB, false>(node, op, noisy_policy);
//		}
	}

	PUCTSelector::PUCTSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant),
			exploration_exponent(config.exploration_exponent)
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
//		const float c_puct = exploration_constant;

		if (init_to == "q_head")
		{
			const PUCT_q_head op(node, c_puct);
			if (use_noise)
				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
		}
		else
		{
			float initial_q = 0.0f; // the default is 'init to loss'
			if (init_to == "parent")
				initial_q = node->getValue().getExpectation();
			else
			{
				if (init_to == "draw")
					initial_q = 0.5f;
				else
					initial_q = 0.0f;
			}
			const PUCT op(node, c_puct, initial_q);
			if (use_noise)
				return find_best_edge_impl<PUCT, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
		}
	}

	PUCTfpuSelector::PUCTfpuSelector(const EdgeSelectorConfig &config) :
			init_to(config.init_to),
			noise_type(config.noise_type),
			noise_weight((config.noise_type == "none") ? 0.0f : config.noise_weight),
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> PUCTfpuSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "puct";
		config.init_to = init_to;
		config.noise_type = noise_type;
		config.noise_weight = noise_weight;
		config.exploration_constant = exploration_constant;
		return std::make_unique<PUCTfpuSelector>(config);
	}
	Edge* PUCTfpuSelector::select(const Node *node) noexcept
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

		const float c_fpu = 0.1f;
		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
//		const float c_puct = exploration_constant;

		if (init_to == "q_head")
		{
			const PUCT_q_head op(node, c_puct);
			if (use_noise)
				return find_best_edge_impl<PUCT_q_head, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT_q_head, false>(node, op, noisy_policy);
		}
		else
		{
			float initial_q = 0.0f; // the default is 'init to loss'
			if (init_to == "parent")
				initial_q = node->getValue().getExpectation();
			else
			{
				if (init_to == "draw")
					initial_q = 0.5f;
				else
					initial_q = 0.0f;
			}

			float sum_of_visited_policy = 0.0f;
			for (auto edge = node->begin(); edge < node->end(); edge++)
				if (edge->getVisits() > 0)
					sum_of_visited_policy += edge->getPolicyPrior();

			initial_q -= c_fpu * std::sqrt(sum_of_visited_policy);

			const PUCT op(node, c_puct, initial_q);
			if (use_noise)
				return find_best_edge_impl<PUCT, true>(node, op, noisy_policy);
			else
				return find_best_edge_impl<PUCT, false>(node, op, noisy_policy);
		}
	}

	UCBSelector::UCBSelector(const EdgeSelectorConfig &config) :
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> UCBSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "lcb";
		config.exploration_constant = exploration_constant;
		return std::make_unique<UCBSelector>(config);
	}
	Edge* UCBSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const float c_puct = 0.25f + 0.073f * std::log(node->getVisits() + node->getVirtualLoss());
		const UCB op(node, c_puct, node->getValue().getExpectation());
		return find_best_edge<UCB>(node, op);
	}

	LCBSelector::LCBSelector(const EdgeSelectorConfig &config) :
			exploration_constant(config.exploration_constant)
	{
	}
	std::unique_ptr<EdgeSelector> LCBSelector::clone() const
	{
		EdgeSelectorConfig config;
		config.policy = "lcb";
		config.exploration_constant = exploration_constant;
		return std::make_unique<LCBSelector>(config);
	}
	Edge* LCBSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		const LCB op(node, exploration_constant);
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

	MaxValueSelector::MaxValueSelector() noexcept
	{
	}
	MaxValueSelector::MaxValueSelector(const EdgeSelectorConfig &config)
	{
	}
	std::unique_ptr<EdgeSelector> MaxValueSelector::clone() const
	{
		return std::make_unique<MaxValueSelector>();
	}
	Edge* MaxValueSelector::select(const Node *node) noexcept
	{
		const MaxValue op;
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

	BestEdgeSelector::BestEdgeSelector()
	{
	}
	BestEdgeSelector::BestEdgeSelector(const EdgeSelectorConfig &config)
	{
	}
	std::unique_ptr<EdgeSelector> BestEdgeSelector::clone() const
	{
		return std::make_unique<BestEdgeSelector>();
	}
	Edge* BestEdgeSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const BestEdge op(node);
		return find_best_edge(node, op);
	}

} /* namespace ag */
