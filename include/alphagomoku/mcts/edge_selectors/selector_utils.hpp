/*
 * selector_utils.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_SELECTORS_SELECTOR_UTILS_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_SELECTORS_SELECTOR_UTILS_HPP_

#include <alphagomoku/mcts/Edge.hpp>

#include <cassert>

namespace ag
{
	[[maybe_unused]] static float getVirtualLoss(const Edge *edge) noexcept
	{
		assert(edge != nullptr);
		const float visits = 1.0e-8f + edge->getVisits();
		const float virtual_loss = edge->getVirtualLoss();
		return visits / (visits + virtual_loss);
	}
	template<typename T>
	float getQ(const T *n, float styleFactor = 0.5f) noexcept
	{
		assert(n != nullptr);
		return n->getWinRate() + styleFactor * n->getDrawRate();
	}
	template<typename T>
	float getProvenQ(const T *node) noexcept
	{
		assert(node != nullptr);
		const bool is_win = node->getProvenValue() == ProvenValue::WIN;
		const bool is_loss = node->getProvenValue() == ProvenValue::LOSS;
		return static_cast<int>(is_win) - static_cast<int>(is_loss);
	}
	template<typename T>
	float getBalance(const T *node) noexcept
	{
		assert(node != nullptr);
		return 1.0f - fabsf(node->getWinRate() - node->getLossRate());
	}
}

#endif /* ALPHAGOMOKU_MCTS_EDGE_SELECTORS_SELECTOR_UTILS_HPP_ */
