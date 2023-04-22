/*
 * EdgeSelector.hpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_

#include <alphagomoku/utils/matrix.hpp>

#include <memory>
#include <vector>
#include <random>

namespace ag
{
	class Move;
	class Node;
	class Edge;
}

namespace ag
{
	/**
	 * @brief Base interface responsible for selecting edge within the tree.
	 */
	class EdgeSelector
	{
		public:
			EdgeSelector() = default;
			EdgeSelector(const EdgeSelector &other) = delete;
			EdgeSelector(EdgeSelector &&other) = delete;
			EdgeSelector& operator=(const EdgeSelector &other) = delete;
			EdgeSelector& operator=(EdgeSelector &&other) = delete;
			virtual ~EdgeSelector() = default;

			virtual std::unique_ptr<EdgeSelector> clone() const = 0;
			virtual Edge* select(const Node *node) noexcept = 0;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class PUCTSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			PUCTSelector(float exploration, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};
	class PUCTSelector_parent: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float exploration_exponent;
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			PUCTSelector_parent(float exploration_c, float exploration_exp, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};
	/**
	 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw) but applies noise to the policy priors.
	 */
	class NoisyPUCTSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const int root_depth;
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
			bool is_initialized = false;
		public:
			NoisyPUCTSelector(int rootDepth, float exploration, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief UCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class UCTSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			UCTSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector used to find few balanced moves, then continues with the baseSelector.
	 */
	class BalancedSelector: public EdgeSelector
	{
		private:
			const int balance_depth;
			std::unique_ptr<EdgeSelector> base_selector;
		public:
			BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
	 */
	class MaxValueSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			MaxValueSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};
	class MaxPolicySelector: public EdgeSelector
	{
		public:
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the most visits.
	 */
	class MaxVisitSelector: public EdgeSelector
	{
		public:
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};
	class MinVisitSelector: public EdgeSelector
	{
		public:
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge that is considered to be the best.
	 */
	class BestEdgeSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			BestEdgeSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_ */
