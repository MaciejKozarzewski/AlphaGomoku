/*
 * TreePolicy.hpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_TREEPOLICY_HPP_
#define ALPHAGOMOKU_MCTS_TREEPOLICY_HPP_

#include <memory>

namespace ag
{
	class Node;
	class Edge;
}

namespace ag
{
	/**
	 * @brief Base interface responsible for searching the tree from root to leaf.
	 */
	class TreePolicy
	{
		public:
			TreePolicy() = default;
			TreePolicy(const TreePolicy &other) = delete;
			TreePolicy(TreePolicy &&other) = delete;
			TreePolicy& operator=(const TreePolicy &other) = delete;
			TreePolicy& operator=(TreePolicy &&other) = delete;
			virtual ~TreePolicy() = default;

			virtual TreePolicy* clone() const = 0;
			virtual Edge* select(const Node *node) noexcept = 0;
	};

	class PuctPolicy: public TreePolicy
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			/**
			 * @brief PUCT tree policy that optimizes P(win) + styleFactor * P(draw)
			 */
			PuctPolicy(float exploration, float styleFactor);
			PuctPolicy* clone() const;
			Edge* select(const Node *node) noexcept;
	};

	class BalancedPolicy: public TreePolicy
	{
		private:
			const uint32_t balance_depth;
			std::unique_ptr<TreePolicy> base_policy;
		public:
			/**
			 * @brief Tree policy used to find few balanced moves, then continues with the basePolicy
			 */
			BalancedPolicy(int balanceDepth, const TreePolicy &basePolicy);
			BalancedPolicy* clone() const;
			Edge* select(const Node *node) noexcept;
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREEPOLICY_HPP_ */
