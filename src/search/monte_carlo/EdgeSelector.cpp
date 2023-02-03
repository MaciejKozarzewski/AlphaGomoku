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

	float getVirtualLoss(const Edge *edge) noexcept
	{
		assert(edge != nullptr);
		const float visits = 1.0e-8f + edge->getVisits();
		const float virtual_loss = edge->getVirtualLoss();
		return visits / (visits + virtual_loss);
	}
	template<typename T>
	float getQ(const T *n, float styleFactor = 0.5f) noexcept
	{
		assert(n != nullptr);
		return n->getWinRate() + styleFactor * n->getDrawRate();
	}
	template<typename T>
	float getProvenQ(const T *node) noexcept
	{
		assert(node != nullptr);
		return static_cast<int>(node->getScore().isWin()) - static_cast<int>(node->getScore().isLoss());
	}
	template<typename T>
	float getBalance(const T *node) noexcept
	{
		assert(node != nullptr);
		return 1.0f - fabsf(node->getWinRate() - node->getLossRate());
	}

	int visits_plus_virtual_loss(const Edge *edge) noexcept
	{
		return edge->getVisits() + edge->getVirtualLoss() + 1000000 * edge->isProven();
	}
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
	float get_q(const Edge *edge) noexcept
	{
		return (edge->getVisits() > 0 or edge->isProven()) ? edge->getValue().getExpectation() : 0.0f;
	}
	int halve(int actionsLeft, int maxEdges, int numUnprovenEdges) noexcept
	{
		assert(actionsLeft > 1);
		actionsLeft -= actionsLeft / 2;
		return std::min(maxEdges, std::min(numUnprovenEdges, actionsLeft));
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
				return edge->getValue().getExpectation();
				case ProvenValue::WIN:
				return Value::win().getExpectation();
			}
		};

		int max_N = 0.0f;
		float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;

		float V_mix;
		if (node->getVisits() == 1)
			V_mix = node->getValue().getExpectation();
		else
		{
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (edge->getVisits() > 0 or edge->isProven())
				{
					sum_v += edge->getValue().getExpectation() * edge->getVisits();
					sum_q += edge->getPolicyPrior() * get_expectation(edge);
					sum_p += edge->getPolicyPrior();
					max_N = std::max(max_N, edge->getVisits());
				}
			const float inv_N = 1.0f / node->getVisits();
			V_mix = (node->getValue().getExpectation() - sum_v * inv_N) + (1.0f - inv_N) * sum_q / sum_p;
		}

		result.clear();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			float Q;
			switch (edge->getScore().getProvenValue())
			{
				case ProvenValue::LOSS:
					Q = -100.0f + 0.1f * edge->getScore().getDistanceToWinOrLoss();
					break;
				case ProvenValue::DRAW:
					Q = Value::draw().getExpectation();
					break;
				default:
				case ProvenValue::UNKNOWN:
					Q = (edge->getVisits() > 0) ? edge->getValue().getExpectation() : V_mix;
					break;
				case ProvenValue::WIN:
					Q = +101.0f - 0.1f * edge->getScore().getDistanceToWinOrLoss();
					break;
			}
			const float sigma_Q = (cVisit + max_N) * cScale * Q;
			const float logit = safe_log(edge->getPolicyPrior());
			result.push_back(logit + sigma_Q);
		}

		softmax(result);
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
			float operator()(const Edge *edge) const noexcept
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge);
				const float U = exploration_constant * sqrtf(parent_log_visit / (1.0f + edge->getVisits()));
				const float P = edge->getPolicyPrior() / (1.0f + edge->getVisits());
				return Q + U + P;
			}
	};
	struct PUCT
	{
			const float parent_sqrt_visit;
			const float style_factor;
			PUCT(int parentVisits, float explorationConstant, float styleFactor) noexcept :
					parent_sqrt_visit(explorationConstant * sqrtf(parentVisits)),
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge *edge) const noexcept
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge);
				const float U = edge->getPolicyPrior() * parent_sqrt_visit / (1.0f + edge->getVisits());
				return Q + U;
			}
	};
	struct NoisyPUCT
	{
			const float parent_sqrt_visit;
			const float style_factor;
			const std::vector<float> &noisy_policy;
			const Edge *first;
			NoisyPUCT(int parentVisits, float explorationConstant, float styleFactor, const std::vector<float> &noisyPolicy,
					const Edge *firstEdge) noexcept :
					parent_sqrt_visit(explorationConstant * sqrtf(parentVisits)),
					style_factor(styleFactor),
					noisy_policy(noisyPolicy),
					first(firstEdge)
			{
			}
			float operator()(const Edge *edge) const noexcept
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge);
				const float U = noisy_policy[std::distance(first, edge)] * parent_sqrt_visit / (1.0f + edge->getVisits());
				return Q + U;
			}
	};
	struct MaxValue
	{
			const float style_factor;
			MaxValue(float styleFactor) noexcept :
					style_factor(styleFactor)
			{
			}
			float operator()(const Edge *edge) const noexcept
			{
				return getQ(edge, style_factor);
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
			float operator()(const Edge *edge) const noexcept
			{
				return edge->getVisits() + getQ(edge, style_factor) * parent_visits + 0.001f * edge->getPolicyPrior();
			}
	};

	template<class Op>
	struct EdgeComparator
	{
			Op op;
			EdgeComparator(Op o) noexcept :
					op(o)
			{
			}
			bool operator()(const Edge *lhs, const Edge *rhs) const noexcept
			{
				if (lhs->isProven() or rhs->isProven())
					return lhs->getScore() > rhs->getScore();
				else
					return op(lhs) > op(rhs);
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

		const PUCT op(node->getVisits(), exploration_constant, style_factor);
		const EdgeComparator<PUCT> greater_than(op);

		Edge *best_edge = node->begin();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (greater_than(edge, best_edge))
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
			if (greater_than(edge, best_edge))
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
			if (node != current_root)
			{ // reset noise
				current_root = node;
				float max_policy_value = std::numeric_limits<float>::lowest();
				for (Edge *iter = node->begin(); iter < node->end(); iter++)
					max_policy_value = std::max(max_policy_value, iter->getPolicyPrior());
				noisy_policy.clear();
				for (Edge *iter = node->begin(); iter < node->end(); iter++)
					noisy_policy.push_back(safe_log(iter->getPolicyPrior()) + max_policy_value * noise(generator));
				softmax(noisy_policy);
			}

			const NoisyPUCT op(node->getVisits(), exploration_constant, style_factor, noisy_policy, node->begin());
			const EdgeComparator<NoisyPUCT> greater_than(op);

			Edge *best_edge = node->begin();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (greater_than(edge, best_edge))
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
			Edge *selected = nullptr;
			float bestValue = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
			{
				const float Q = getBalance(edge) * getVirtualLoss(edge) - 10.0f * edge->isProven();
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
			if (greater_than(edge, best_edge))
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
			if (greater_than(edge, best_edge))
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
	void SequentialHalvingSelector::initialize(const Node *newRoot)
	{
		assert(newRoot != nullptr);
		assert(newRoot->isRoot());
		expected_visit_count = 0;
		simulations_left = max_simulations - 1; // one simulation is used to evaluate and expand the root node
		number_of_actions_left = newRoot->numberOfEdges();

		action_list.clear();
		for (Edge *iter = newRoot->begin(); iter < newRoot->end(); iter++)
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
		if (number_of_actions_left > 2)
		{
			const bool is_level_complete = std::all_of(action_list.begin(), action_list.begin() + number_of_actions_left, [this](const Data &data)
			{	return (data.edge->getVisits() >= expected_visit_count) or data.edge->isProven();});

			if (is_level_complete)
			{ // advance to the next level
				const int num_unproven_edges = std::count_if(action_list.begin(), action_list.end(), [this](const Data &data)
				{	return not data.edge->isProven();});
//				std::cout << "moving to the next level with " << number_of_actions_left << " actions (" << num_unproven_edges << " unproven)\n";

				number_of_actions_left = halve(number_of_actions_left, max_edges, num_unproven_edges);
//				std::cout << number_of_actions_left << " actions left\n";
				sort_workspace();

				const int levels_left = std::max(1, integer_log2(number_of_actions_left) - 1); // the last level is when we have just 2 actions left
				const int simulations_for_this_level = simulations_left / levels_left;
				expected_visit_count += std::max(1, simulations_for_this_level / number_of_actions_left);

//				std::cout << "levels left = " << levels_left << ", simulations left = " << simulations_left << ", simulations for this level = "
//						<< simulations_for_this_level << ", expected visit count = " << expected_visit_count << '\n';
//
//				std::cout << "action list:\n";
//				const int max_N = std::max_element(current_root->begin(), current_root->end(), [](const Edge &lhs, const Edge &rhs)
//				{	return lhs.getVisits() < rhs.getVisits();})->getVisits();
//				const float scale = (C_visit + max_N) * C_scale;
//				for (int i = 0; i < number_of_actions_left; i++)
//					std::cout << action_list[i].edge->toString() << " : noise=" << action_list[i].noise << " : logit=" << action_list[i].logit
//							<< " : sigma(Q)=" << scale * get_q(action_list[i].edge) << '\n';
//
//				std::cout << "...\n";
//				for (size_t i = number_of_actions_left; i < action_list.size(); i++)
//					if (action_list[i].edge->isProven())
//						std::cout << action_list[i].edge->toString() << " : noise=" << action_list[i].noise << " : logit=" << action_list[i].logit
//								<< " : sigma(Q)=" << scale * get_q(action_list[i].edge) << '\n';
			}
		}

		auto result = std::min_element(action_list.begin(), action_list.begin() + number_of_actions_left, [](const Data &lhs, const Data &rhs)
		{	return visits_plus_virtual_loss(lhs.edge) < visits_plus_virtual_loss(rhs.edge);});
		assert(result != action_list.end());

		simulations_left--;
		return result->edge;
	}
	Edge* SequentialHalvingSelector::select_below_root(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		create_completed_Q(workspace, node, C_visit, C_scale);

		const float inv_visits = 1.0f / (1 + node->getVisits());

		Edge *selected = nullptr;
		float max_diff = std::numeric_limits<float>::lowest();
		for (int i = 0; i < node->numberOfEdges(); i++)
		{
			const float diff = (workspace[i] - node->getEdge(i).getVisits() * inv_visits) * getVirtualLoss(node->begin() + i);
			if (diff > max_diff)
			{
				selected = node->begin() + i;
				max_diff = diff;
			}
		}
		assert(selected != nullptr);
		return selected;

//		return PUCTSelector(1.0f).select(node);
	}
	void SequentialHalvingSelector::sort_workspace() noexcept
	{
		int max_N = 0;
		for (auto iter = action_list.begin(); iter < action_list.end(); iter++)
			max_N = std::max(max_N, iter->edge->getVisits());

		const float scale = (C_visit + max_N) * C_scale;
		std::partial_sort(action_list.begin(), action_list.begin() + number_of_actions_left, action_list.end(),
				[scale](const Data &lhs, const Data &rhs)
				{
					const float lhs_value = lhs.noise + lhs.logit + scale * get_q(lhs.edge) - 1.0e6f * lhs.edge->isProven();
					const float rhs_value = rhs.noise + rhs.logit + scale * get_q(rhs.edge) - 1.0e6f * rhs.edge->isProven();
					return lhs_value > rhs_value; // intentionally inverted to sort in descending order
					});
	}

} /* namespace ag */
