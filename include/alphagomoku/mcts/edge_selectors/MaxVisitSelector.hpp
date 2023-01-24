/*
 * MaxVisitSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVISITSELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVISITSELECTOR_HPP_

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
	 * @brief Edge selector that chooses edge with the most visits.
	 */
	class MaxVisitSelector: public EdgeSelector
	{
		public:
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_MAXVISITSELECTOR_HPP_ */
