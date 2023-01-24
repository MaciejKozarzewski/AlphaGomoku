/*
 * PUCTSelector.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_PUCTSELECTOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_PUCTSELECTOR_HPP_

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
			Edge* select(const Node *node) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_PUCTSELECTOR_HPP_ */
