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

#include <random>
#include <cassert>

namespace
{
	float getVirtualLoss(const ag::Edge *edge) noexcept
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

	using namespace ag;
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
		return (edge->getVisits() > 0) ? edge->getValue().getExpectation() : 0.0f;
	}
	int halve(int actionsLeft, int maxEdges, int numUnprovenEdges) noexcept
	{
		assert(actionsLeft > 1);
		actionsLeft -= actionsLeft / 2;
		return std::min(maxEdges, std::min(numUnprovenEdges, actionsLeft));
	}
	void softmax(std::vector<float> &vec) noexcept
	{
		float max = std::numeric_limits<float>::lowest();
		for (size_t i = 0; i < vec.size(); i++)
			max = std::max(max, vec[i]);
		float sum = 0.0f;
		for (size_t i = 0; i < vec.size(); i++)
		{
			vec[i] = std::exp(vec[i] - max);
			sum += vec[i];
		}
		assert(sum != 0.0f);
		sum = 1.0f / sum;
		for (size_t i = 0; i < vec.size(); i++)
			vec[i] *= sum;
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
		const float parent_value = getQ(node, style_factor);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());
		const float cbrt_visit = exploration_constant * cbrtf(node->getVisits());
		const float log_visit = exploration_constant * logf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
//				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // use action value head
				const float Q = (edge->getVisits() > 0) ? getQ(edge, style_factor) * getVirtualLoss(edge) : parent_value; // init to parent
//				const float Q = (edge->getVisits() > 0) ? getQ(edge, style_factor) * getVirtualLoss(edge) : 0.0f; // init to loss
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

	QHeadSelector::QHeadSelector(float exploration, float styleFactor) :
			exploration_constant(exploration),
			style_factor(styleFactor)
	{
	}
	std::unique_ptr<EdgeSelector> QHeadSelector::clone() const
	{
		return std::make_unique<QHeadSelector>(exploration_constant, style_factor);
	}
	Edge* QHeadSelector::select(const Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // use action value head
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
		const float parent_value = getQ(node, style_factor);
		const float log_visit = logf(node->getVisits());

		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			if (edge->isProven() == false)
			{
//				const float Q = (edge->getVisits() > 0) ? getQ(edge, style_factor) * getVirtualLoss(edge) : parent_value; // init to parent
				const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // init to loss
				const float U = exploration_constant * sqrtf(log_visit / (1.0f + edge->getVisits()));
				const float P = edge->getPolicyPrior() / (1.0f + edge->getVisits());

				if (Q + U + P > bestValue)
				{
					selected = edge;
					bestValue = Q + U + P;
				}
			}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

	NoisyPUCTSelector::NoisyPUCTSelector(float exploration, float styleFactor) :
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
#ifndef NDEBUG
				std::default_random_engine generator;
#else
				std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
#endif
				std::extreme_value_distribution<float> noise;
				noisy_policy.clear();
				for (Edge *iter = node->begin(); iter < node->end(); iter++)
					noisy_policy.push_back(safe_log(iter->getPolicyPrior()) + noise(generator));
				softmax(noisy_policy);
			}

			const float sqrt_visit = exploration_constant * sqrtf(node->getVisits());
			Edge *selected = nullptr;
			float bestValue = std::numeric_limits<float>::lowest();
			for (Edge *edge = node->begin(); edge < node->end(); edge++)
				if (edge->isProven() == false)
				{
					const float Q = getQ(edge, style_factor) * getVirtualLoss(edge); // use action value head
					const float U = noisy_policy[std::distance(node->begin(), edge)] * sqrt_visit / (1.0f + edge->getVisits()); // classical PUCT formula

					if (Q + U > bestValue)
					{
						selected = edge;
						bestValue = Q + U;
					}
				}
			assert(selected != nullptr); // there should always be some best edge
			return selected;
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
		Edge *selected = nullptr;
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			const float Q = getQ(edge, style_factor) + 1.0e6f * getProvenQ(edge);
			if (Q > bestValue)
			{
				selected = edge;
				bestValue = Q;
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
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
		float bestValue = std::numeric_limits<float>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			if (edge->getVisits() > bestValue)
			{
				selected = edge;
				bestValue = edge->getVisits();
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
		Edge *selected = nullptr;
		double bestValue = std::numeric_limits<double>::lowest();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			double value = edge->getVisits() + getQ(edge, style_factor) * node->getVisits() + 0.001 * edge->getPolicyPrior()
					+ getProvenQ(edge) * 1.0e9;
			if (value > bestValue)
			{
				selected = edge;
				bestValue = value;
			}
		}
		assert(selected != nullptr); // there should always be some best edge
		return selected;
	}

	SequentialHalvingSelector::SequentialHalvingSelector(int maxEdges, int maxSimulations, float cVisit, float cScale) :
			max_edges(maxEdges),
			max_simulations(maxSimulations),
			C_visit(cVisit),
			C_scale(cScale)
	{
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
			if (node != current_root)
				reset(node);

			return select_at_root(node);
		}
		else
			return select_below_root(node);
	}
	/*
	 * private
	 */
	void SequentialHalvingSelector::reset(const Node *newRoot)
	{
		assert(newRoot != current_root);
		assert(newRoot != nullptr);
		assert(newRoot->isRoot());
		current_root = newRoot;
		expected_visit_count = 0;
		simulations_left = max_simulations - 1; // one simulation is used to evaluate and expand the root node
		number_of_actions_left = newRoot->numberOfEdges();

#ifndef NDEBUG
		std::default_random_engine generator;
#else
		std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
#endif
		std::extreme_value_distribution<float> noise;
		action_list.clear();
		for (Edge *iter = newRoot->begin(); iter < newRoot->end(); iter++)
		{
			Data tmp;
			tmp.edge = iter;
			tmp.noise = noise(generator);
			tmp.logit = safe_log(iter->getPolicyPrior());
			action_list.push_back(tmp);
		}
//		std::cout << "reseting\n";
	}
	Edge* SequentialHalvingSelector::select_at_root(const Node *node) noexcept
	{
//		std::cout << "select_at_root()\n";
		const bool is_level_complete = std::all_of(action_list.begin(), action_list.begin() + number_of_actions_left, [this](const Data &data)
		{	return (data.edge->getVisits() >= expected_visit_count) or data.edge->isProven();});

		if (is_level_complete)
		{
//			std::cout << "level is complete\n";
			if (number_of_actions_left > 1)
			{ // advance to the next level
				const int num_unproven_edges = std::count_if(action_list.begin(), action_list.end(), [this](const Data &data)
				{	return not data.edge->isProven();});
//				std::cout << "moving to the next level with " << number_of_actions_left << " actions (" << num_unproven_edges << " unproven)\n";

				number_of_actions_left = halve(number_of_actions_left, max_edges, num_unproven_edges);
//				std::cout << number_of_actions_left << " actions left\n";
				sort_workspace();

				if (number_of_actions_left > 1)
				{
					const int levels_left = integer_log2(number_of_actions_left) - 1; // the last level is when we have just 2 actions left
					const int simulations_for_this_level = simulations_left / levels_left;
					expected_visit_count += std::max(1, simulations_for_this_level / number_of_actions_left);

//					std::cout << "levels left = " << levels_left << ", simulations left = " << simulations_left << ", simulations for this level = "
//							<< simulations_for_this_level << ", expected visit count = " << expected_visit_count << '\n';
				}
			}
			if (number_of_actions_left == 1)
			{
//				std::cout << "final action:\n" << action_list.front().edge->toString() << '\n';
				return action_list.front().edge; // after reaching final level we always return the same best edge
			}
		}

		auto result = std::min_element(action_list.begin(), action_list.begin() + number_of_actions_left, [](const Data &lhs, const Data &rhs)
		{	return visits_plus_virtual_loss(lhs.edge) < visits_plus_virtual_loss(rhs.edge);});
		assert(result != action_list.end());

//		if (is_level_complete) // TODO delete this block later
//		{
//			std::cout << "action list:\n";
//			const int max_N = std::max_element(current_root->begin(), current_root->end(), [](const Edge &lhs, const Edge &rhs)
//			{	return lhs.getVisits() < rhs.getVisits();})->getVisits();
//			const float scale = (C_visit + max_N) * C_scale;
//			for (int i = 0; i < number_of_actions_left; i++)
//				std::cout << action_list[i].edge->toString() << " : noise=" << action_list[i].noise << " : logit=" << action_list[i].logit
//						<< " : sigma(Q)=" << scale * get_q(action_list[i].edge) << '\n';
//
//			std::cout << "...\n";
//			for (size_t i = number_of_actions_left; i < action_list.size(); i++)
//				if (action_list[i].edge->isProven())
//					std::cout << action_list[i].edge->toString() << " : noise=" << action_list[i].noise << " : logit=" << action_list[i].logit
//							<< " : sigma(Q)=" << scale * get_q(action_list[i].edge) << '\n';
//		}
//		std::cout << "--> --> selected : " << result->edge->toString() << '\n';

		simulations_left--;
		return result->edge;
	}
	Edge* SequentialHalvingSelector::select_below_root(const Node *node) noexcept
	{
//		int max_N = 0.0f;
//		float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;
//		for (Edge *edge = node->begin(); edge < node->end(); edge++)
//			if (edge->getVisits() > 0)
//			{
//				sum_v += getQ(edge) * edge->getVisits();
//				sum_q += edge->getPolicyPrior() * getQ(edge) * getVirtualLoss(edge);
//				sum_p += edge->getPolicyPrior();
//				max_N = std::max(max_N, edge->getVisits());
//			}
//		const float inv_N = 1.0f / node->getVisits();
//		float V_mix = getQ(node) - sum_v * inv_N + (1.0f - inv_N) / sum_p * sum_q;
//
//		workspace.clear();
//		for (Edge *edge = node->begin(); edge < node->end(); edge++)
//		{
//			const float q = (edge->getVisits() > 0) ? getQ(edge) : V_mix;
//		}
//		std::cout << node->toString() << "\n";
//		for (auto e = node->begin(); e < node->end(); e++)
//			std::cout << "    " << e->toString() << '\n';

		return UCTSelector(1.0f).select(node);
	}
	void SequentialHalvingSelector::sort_workspace() noexcept
	{
		assert(current_root != nullptr);
		const int max_N = std::max_element(current_root->begin(), current_root->end(), [](const Edge &lhs, const Edge &rhs)
		{	return lhs.getVisits() < rhs.getVisits();})->getVisits();

		const float scale = (C_visit + max_N) * C_scale;
		std::sort(action_list.begin(), action_list.end(), [scale](const Data &lhs, const Data &rhs)
		{
			const float lhs_value = lhs.noise + lhs.logit + scale * get_q(lhs.edge) - 1.0e6f * lhs.edge->isProven();
			const float rhs_value = rhs.noise + rhs.logit + scale * get_q(rhs.edge) - 1.0e6f * rhs.edge->isProven();
			return lhs_value > rhs_value; // intentionally inverted to sort in descending order
			});
	}

} /* namespace ag */
