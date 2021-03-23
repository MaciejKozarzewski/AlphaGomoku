/*
 * game_rules.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_GAME_RULES_HPP_
#define ALPHAGOMOKU_UTILS_GAME_RULES_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	enum class GameRules
	{
		FREESTYLE, STANDARD, RENJU, CARO
	};

	enum class GameOutcome
	{
		UNKNOWN, DRAW, CROSS_WIN, CIRCLE_WIN
	};

	GameRules rulesFromString(const std::string &str);
	std::string toString(GameRules rules);

	GameOutcome outcomeFromString(const std::string &str);
	std::string toString(GameOutcome outcome);

	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board);
	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board, const Move &last_move);
}

#endif /* ALPHAGOMOKU_UTILS_GAME_RULES_HPP_ */
