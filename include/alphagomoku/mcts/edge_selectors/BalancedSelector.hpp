/*
 * BalancedSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BALANCEDSELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BALANCEDSELECTOR_HPP_

#include <alphagomoku/mcts/edge_selectors/EdgeSelector.hpp>

#include <memory>

namespace ag
{
	class Node;
	class Edge;
}

namespace ag
{
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
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BALANCEDSELECTOR_HPP_ */
