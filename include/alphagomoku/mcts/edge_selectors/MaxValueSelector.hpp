/*
 * MaxValueSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVALUESELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVALUESELECTOR_HPP_

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
	 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
	 */
	class MaxValueSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			MaxValueSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVALUESELECTOR_HPP_ */
