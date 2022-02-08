/*
 * EdgeSelector.hpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGESELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGESELECTOR_HPP_

#include <memory>

namespace ag
{
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

			virtual EdgeSelector* clone() const = 0;
			virtual Edge* select(const Node *node) const noexcept = 0;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class PuctSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:

			PuctSelector(float exploration, float styleFactor = 0.5f);
			PuctSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
	};

	/**
	 * @brief UCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class UctSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:

			UctSelector(float exploration, float styleFactor = 0.5f);
			UctSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
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
			BalancedSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
	 */
	class ValueSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			ValueSelector(float styleFactor = 0.5f);
			ValueSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the most visits.
	 */
	class VisitSelector: public EdgeSelector
	{
		public:
			VisitSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
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
			BestEdgeSelector* clone() const;
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGESELECTOR_HPP_ */
