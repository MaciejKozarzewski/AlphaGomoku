/*
 * caro.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_RULES_CARO_HPP_
#define ALPHAGOMOKU_RULES_CARO_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{
	GameOutcome getOutcomeCaro(const matrix<Sign> &board);
	GameOutcome getOutcomeCaro(const matrix<Sign> &board, const Move &last_move);

	GameOutcome getOutcomeCaro(const std::vector<Sign> &line);
} /* namespace ag */

#endif /* ALPHAGOMOKU_RULES_CARO_HPP_ */
