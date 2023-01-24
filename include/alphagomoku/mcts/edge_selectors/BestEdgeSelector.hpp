/*
 * BestEdgeSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BESTEDGESELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BESTEDGESELECTOR_HPP_

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
	 * @brief Edge selector that chooses edge that is considered to be the best.
	 */
	class BestEdgeSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			BestEdgeSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_BESTEDGESELECTOR_HPP_ */
