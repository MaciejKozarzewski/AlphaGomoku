/*
 * rules.hpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_RULES_HPP_
#define ALPHAGOMOKU_GAME_RULES_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Move.hpp>

#include <string>

namespace ag
{
	enum class GameRules
	{
		FREESTYLE,
		STANDARD,
		RENJU,
		CARO5,
		CARO6
	};
	std::string toString(GameRules rules);
	GameRules rulesFromString(const std::string &str);

	enum class GameOutcome
	{
		UNKNOWN,
		DRAW,
		CROSS_WIN, // or black win
		CIRCLE_WIN // or white win
	};
	std::string toString(GameOutcome outcome);
	GameOutcome outcomeFromString(const std::string &str);

	/*
	 * \brief Returns game outcome given board state and last move.
	 * The move may or may not be already placed on board.
	 * If numberOfMovesForDraw is negative it means that we play until board is full.
	 */
	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board, Move lastMove, int numberOfMovesForDraw = -1);
	bool isForbidden(const matrix<Sign> &board, Move move);

} /* namespace ag */

#endif /* ALPHAGOMOKU_GAME_RULES_HPP_ */
