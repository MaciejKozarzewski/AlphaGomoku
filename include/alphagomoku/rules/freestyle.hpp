/*
 * freestyle.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_RULES_FREESTYLE_HPP_
#define ALPHAGOMOKU_RULES_FREESTYLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{
	GameOutcome getOutcomeFreestyle(const matrix<Sign> &board);
	GameOutcome getOutcomeFreestyle(const matrix<Sign> &board, const Move &last_move);

	GameOutcome getOutcomeFreestyle(const std::vector<Sign> &line);
} /* namespace ag */

#endif /* ALPHAGOMOKU_RULES_FREESTYLE_HPP_ */
