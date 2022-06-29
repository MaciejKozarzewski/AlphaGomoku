/*
 * rules.hpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_RULES_HPP_
#define ALPHAGOMOKU_GAME_RULES_HPP_

#include <string>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{
	enum class GameRules
	{
		FREESTYLE,
		STANDARD,
		RENJU,
		CARO
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

	GameOutcome getOutcome_v2(GameRules rules, matrix<Sign> &board, Move lastMove);
	bool isForbidden(matrix<Sign> &board, Move move);

} /* namespace ag */

#endif /* ALPHAGOMOKU_GAME_RULES_HPP_ */
