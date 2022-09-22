/*
 * renju.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/game/Board.hpp>

namespace ag
{
	GameOutcome getOutcomeRenju(const matrix<Sign> &board)
	{
		return GameOutcome::UNKNOWN; // TODO
	}
	GameOutcome getOutcomeRenju(const matrix<Sign> &board, const Move &last_move)
	{
		// TODO
		if (Board::isFull(board))
			return GameOutcome::DRAW;
		else
			return GameOutcome::UNKNOWN;
	}

	GameOutcome getOutcomeRenju(const std::vector<Sign> &line)
	{
		return GameOutcome::UNKNOWN; // TODO
	}
}

