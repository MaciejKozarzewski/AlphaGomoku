/*
 * renju.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	GameOutcome getOutcomeRenju(const matrix<Sign> &board)
	{
		return GameOutcome::UNKNOWN; // TODO
	}
	GameOutcome getOutcomeRenju(const matrix<Sign> &board, const Move &last_move)
	{
		// TODO
		if (isBoardFull(board))
			return GameOutcome::DRAW;
		else
			return GameOutcome::UNKNOWN;
	}
}

