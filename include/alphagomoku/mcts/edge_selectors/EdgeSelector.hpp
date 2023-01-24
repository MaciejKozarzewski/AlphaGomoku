/*
 * EdgeSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_EDGESELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_EDGESELECTOR_HPP_

#include <memory>

namespace ag
{
	class Node;
	class Edge;
}

namespace ag
{
	/**
	 * @brief Base interface responsible for selecting edges within the tree.
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
			virtual Edge* select(const Node *node) const noexcept = 0;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_EDGESELECTOR_HPP_ */
