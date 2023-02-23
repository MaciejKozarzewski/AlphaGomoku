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
	int visitsPlusVirtualLoss(const Edge &edge) noexcept
	{
		return edge.getVisits() + edge.getVirtualLoss() + 1000000 * edge.isProven();
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
			const float parent_sqrt_visit;
			const float style_factor;
			PUCT(const Node &parent, float explorationConstant, float styleFactor) noexcept :
					parent_sqrt_visit(explorationConstant * sqrtf(parent.getVisits())),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = edge.getExpectation(style_factor);
				const float U = edge.getPolicyPrior() * parent_sqrt_visit / (1.0f + edge.getVisits());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct NoisyPUCT
	{
			const float parent_sqrt_visit;
			const float style_factor;
			const std::vector<float> &noisy_policy;
			const Edge *first;
			NoisyPUCT(const Node &parent, float explorationConstant, float styleFactor, const std::vector<float> &noisyPolicy) noexcept :
					parent_sqrt_visit(explorationConstant * sqrtf(parent.getVisits())),
					style_factor(styleFactor),
					noisy_policy(noisyPolicy),
					first(parent.begin())
			{
			}
			float operator()(const Edge &edge) const noexcept
			{
				const float Q = edge.getExpectation(style_factor);
				const float U = noisy_policy[std::distance(first, &edge)] * parent_sqrt_visit / (1.0f + edge.getVisits());
				return Q * getVirtualLoss(edge) + U;
			}
	};
	struct MaxValue
	{
			const float style_factor;
			const int min_visit_count;
			MaxValue(float styleFactor, int minVisitCount = -1) noexcept :
					style_factor(styleFactor),
					min_visit_count(minVisitCount)
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
						return (edge.getVisits() > min_visit_count) ? edge.getExpectation(style_factor) : 0.0f;
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

	int integer_log2(int x) noexcept
	{
		int result = 0;
		while (x > 0)
		{
			x /= 2;
			result++;
		}
		return result;
	}
	void softmax(std::vector<float> &vec) noexcept
	{
		const float shift = *std::max_element(vec.begin(), vec.end());

		float sum = 0.0f;
		for (size_t i = 0; i < vec.size(); i++)
		{
			float tmp = std::exp(vec[i] - shift);
			if (tmp < 1.0e-9f)
				tmp = 0.0f;
			sum += tmp;
			vec[i] = tmp;
		}
		assert(sum != 0.0f);
		sum = 1.0f / sum;
		for (size_t i = 0; i < vec.size(); i++)
			vec[i] *= sum;
	}
	void create_completed_Q(std::vector<float> &result, const Node *node, float cVisit, float cScale)
	{
		assert(node != nullptr);
		auto get_expectation = [](const Edge *edge)
		{
			switch (edge->getScore().getProvenValue())
			{
				case ProvenValue::LOSS:
				return Value::loss().getExpectation();
				case ProvenValue::DRAW:
				return Value::draw().getExpectation();
				default:
				case ProvenValue::UNKNOWN:
				return edge->getExpectation();
				case ProvenValue::WIN:
				return Value::win().getExpectation();
			}
		};

		int max_N = 0.0f;
		float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;

		float V_mix;
		if (node->getVisits() == 1)
			V_mix = node->getExpectation();
		else
		{
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (edge->getVisits() > 0 or edge->isProven())
				{
					sum_v += edge->getExpectation() * edge->getVisits();
					sum_q += edge->getPolicyPrior() * get_expectation(edge);
					sum_p += edge->getPolicyPrior();
					max_N = std::max(max_N, edge->getVisits());
				}
			const float inv_N = 1.0f / node->getVisits();
			V_mix = (node->getExpectation() - sum_v * inv_N) + (1.0f - inv_N) * sum_q / sum_p;
		}

		result.clear();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q;
			switch (edge->getScore().getProvenValue())
			{
				case ProvenValue::LOSS:
					Q = -100.0f + 0.1f * edge->getScore().getDistance();
					break;
				case ProvenValue::DRAW:
					Q = Value::draw().getExpectation();
					break;
				default:
				case ProvenValue::UNKNOWN:
					Q = (edge->getVisits() > 0) ? edge->getExpectation() : V_mix;
					break;
				case ProvenValue::WIN:
					Q = +101.0f - 0.1f * edge->getScore().getDistance();
					break;
			}
			const float sigma_Q = (cVisit + max_N) * cScale * Q;
			const float logit = safe_log(edge->getPolicyPrior());
			result.push_back(logit + sigma_Q);
		}

		softmax(result);
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

		const PUCT op(*node, exploration_constant, style_factor);
		const EdgeComparator<PUCT> greater_than(op);

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

	NoisyPUCTSelector::NoisyPUCTSelector(float exploration, float styleFactor) :
			generator(std::chrono::system_clock::now().time_since_epoch().count()),
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> NoisyPUCTSelector::clone() const
	{
		return std::make_unique<NoisyPUCTSelector>(exploration_constant, style_factor);
	}
	Edge* NoisyPUCTSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->isRoot())
		{
			if (not is_initialized)
			{ // reset noise
				float max_policy_value = std::numeric_limits<float>::lowest();
				for (Edge *iter = node->begin(); iter < node->end(); iter++)
					max_policy_value = std::max(max_policy_value, iter->getPolicyPrior());
				noisy_policy.clear();
				for (Edge *iter = node->begin(); iter < node->end(); iter++)
					noisy_policy.push_back(safe_log(iter->getPolicyPrior()) + noise(generator));
				softmax(noisy_policy);
				is_initialized = true;
			}

			const NoisyPUCT op(*node, exploration_constant, style_factor, noisy_policy);
			const EdgeComparator<NoisyPUCT> greater_than(op);

			Edge *best_edge = node->begin();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (greater_than(*edge, *best_edge))
					best_edge = edge;
			return best_edge;
		}
		else
			return PUCTSelector(exploration_constant, style_factor).select(node);
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

	SequentialHalvingSelector::SequentialHalvingSelector(int maxEdges, int maxSimulations, float cVisit, float cScale) :
			generator(std::chrono::system_clock::now().time_since_epoch().count()),
			max_edges(maxEdges),
			max_simulations(maxSimulations),
			C_visit(cVisit),
			C_scale(cScale)
	{
		action_list.reserve(512);
		workspace.reserve(512);
		assert(isPowerOf2(max_edges));
		assert(maxSimulations >= 2 * maxEdges - 1);
	}
	std::unique_ptr<EdgeSelector> SequentialHalvingSelector::clone() const
	{
		return std::make_unique<SequentialHalvingSelector>(max_edges, max_simulations, C_visit, C_scale);
	}
	Edge* SequentialHalvingSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->isRoot())
		{
			if (not is_initialized)
				initialize(node);
			return select_at_root(node);
		}
		else
			return select_below_root(node);
	}
	/*
	 * private
	 */
	void SequentialHalvingSelector::initialize(const Node *node)
	{
		assert(node != nullptr);
		assert(node->isRoot());
		is_initialized = true;
		expected_visit_count = 0;
		simulations_left = max_simulations;

		action_list.clear();
		for (Edge *iter = node->begin(); iter < node->end(); iter++)
		{
			Data tmp;
			tmp.edge = iter;
			tmp.noise = noise(generator);
			tmp.logit = safe_log(iter->getPolicyPrior());
			action_list.push_back(tmp);
		}
	}
	Edge* SequentialHalvingSelector::select_at_root(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isRoot());
		assert(node->isLeaf() == false);
		assert(is_initialized);
		if (action_list.size() > 2)
		{
			const bool is_level_complete = std::all_of(action_list.begin(), action_list.end(), [this](const Data &data)
			{	return (data.edge->getVisits() >= expected_visit_count) or data.edge->isProven();});

			if (is_level_complete)
			{ // advance to the next level
//				std::cout << "moving to the next level with " << action_list.size() << " actions\n";

				const int number_of_actions_left = std::min((size_t) max_edges, action_list.size() - action_list.size() / 2);
//				std::cout << number_of_actions_left << " actions left\n";
				sort_workspace(number_of_actions_left);
				action_list.erase(action_list.begin() + number_of_actions_left, action_list.end());

				const int levels_left = std::max(1, integer_log2(number_of_actions_left) - 1); // the last level is when we have just 2 actions left
				const int simulations_for_this_level = simulations_left / levels_left;
				expected_visit_count += std::max(1, simulations_for_this_level / number_of_actions_left);

//				std::cout << "levels left = " << levels_left << ", simulations left = " << simulations_left << ", simulations for this level = "
//						<< simulations_for_this_level << ", expected visit count = " << expected_visit_count << '\n';
//				std::cout << "action list:\n";
//				int max_N = 0;
//				for (auto iter = action_list.begin(); iter < action_list.end(); iter++)
//					max_N = std::max(max_N, iter->edge->getVisits());
//				const float scale = (C_visit + max_N) * C_scale;
//				MaxValue op(0.5f, 0);
//				for (auto iter = action_list.begin(); iter < action_list.end(); iter++)
//					std::cout << iter->edge->toString() << " : noise=" << iter->noise << " : logit=" << iter->logit << " : sigma(Q)="
//							<< scale * op(*(iter->edge)) << '\n';
			}
		}

		auto result = std::min_element(action_list.begin(), action_list.end(), [](const Data &lhs, const Data &rhs)
		{	return visitsPlusVirtualLoss(*lhs.edge) < visitsPlusVirtualLoss(*rhs.edge);});
		assert(result != action_list.end());

		simulations_left--;
		return result->edge;
	}
	Edge* SequentialHalvingSelector::select_below_root(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		assert(is_initialized);

		create_completed_Q(workspace, node, C_visit, C_scale);

		const float inv_visits = 1.0f / (1 + node->getVisits());

		Edge *selected = nullptr;
		float max_diff = std::numeric_limits<float>::lowest();
		for (int i = 0; i < node->numberOfEdges(); i++)
		{
			const float diff = (workspace[i] - node->getEdge(i).getVisits() * inv_visits) * getVirtualLoss(node->getEdge(i));
			if (diff > max_diff)
			{
				selected = node->begin() + i;
				max_diff = diff;
			}
		}
		assert(selected != nullptr);
		return selected;
	}
	void SequentialHalvingSelector::sort_workspace(int topN) noexcept
	{
		int max_N = 0;
		for (auto iter = action_list.begin(); iter < action_list.end(); iter++)
			max_N = std::max(max_N, iter->edge->getVisits());

		const float scale = (C_visit + max_N) * C_scale;
		MaxValue op(0.5f, 0);
		std::partial_sort(action_list.begin(), action_list.begin() + topN, action_list.end(), [scale, op](const Data &lhs, const Data &rhs)
		{
			const float lhs_value = lhs.noise + lhs.logit + scale * op(*lhs.edge);
			const float rhs_value = rhs.noise + rhs.logit + scale * op(*rhs.edge);
			return lhs_value > rhs_value; // intentionally inverted to sort in descending order
			});
	}

} /* namespace ag */
