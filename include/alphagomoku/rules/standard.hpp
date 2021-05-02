/*
 * standard.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_RULES_STANDARD_HPP_
#define ALPHAGOMOKU_RULES_STANDARD_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{
	GameOutcome getOutcomeStandard(const matrix<Sign> &board);
	GameOutcome getOutcomeStandard(const matrix<Sign> &board, const Move &last_move);

	GameOutcome getOutcomeStandard(const std::vector<Sign> &line);
} /* namespace ag */

#endif /* ALPHAGOMOKU_RULES_STANDARD_HPP_ */
