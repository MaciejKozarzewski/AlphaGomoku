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
	 * @brief Base interface responsible for selecting edge within the tree..
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
			virtual Edge* select(const Node *node) noexcept = 0;
	};

	class PuctSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			/**
			 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw).
			 */
			PuctSelector(float exploration, float styleFactor);
			PuctSelector* clone() const;
			Edge* select(const Node *node) noexcept;
	};

	class UctSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			/**
			 * @brief UCT edge selector that optimizes P(win) + styleFactor * P(draw).
			 */
			UctSelector(float exploration, float styleFactor);
			UctSelector* clone() const;
			Edge* select(const Node *node) noexcept;
	};

	class BalancedSelector: public EdgeSelector
	{
		private:
			const uint32_t balance_depth;
			std::unique_ptr<EdgeSelector> base_selector;
		public:
			/**
			 * @brief Edge selector used to find few balanced moves, then continues with the baseSelector.
			 */
			BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector);
			BalancedSelector* clone() const;
			Edge* select(const Node *node) noexcept;
	};

	class ValueSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			/**
			 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
			 */
			ValueSelector(float styleFactor);
			ValueSelector* clone() const;
			Edge* select(const Node *node) noexcept;
	};

	class VisitSelector: public EdgeSelector
	{
		public:
			/**
			 * @brief Edge selector that chooses edge with the most visits.
			 */
			VisitSelector* clone() const;
			Edge* select(const Node *node) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGESELECTOR_HPP_ */
