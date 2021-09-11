/*
 * game_rules.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_RULES_GAME_RULES_HPP_
#define ALPHAGOMOKU_RULES_GAME_RULES_HPP_

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board);
	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board, const Move &last_move);

	GameOutcome getOutcome(GameRules rules, const std::vector<Sign> &line);
}

#endif /* ALPHAGOMOKU_RULES_GAME_RULES_HPP_ */
