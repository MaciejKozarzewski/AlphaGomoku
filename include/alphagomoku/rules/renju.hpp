/*
 * renju.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_RULES_RENJU_HPP_
#define ALPHAGOMOKU_RULES_RENJU_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{
	GameOutcome getOutcomeRenju(const matrix<Sign> &board);
	GameOutcome getOutcomeRenju(const matrix<Sign> &board, const Move &last_move);

	GameOutcome getOutcomeRenju(const std::vector<Sign> &line);
} /* namespace ag */

#endif /* ALPHAGOMOKU_RULES_RENJU_HPP_ */
