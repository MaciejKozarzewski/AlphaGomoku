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

	Sign getWhoWins(GameRules rules, const matrix<Sign> &board);
	Sign getWhoWins(GameRules rules, const matrix<Sign> &board, const Move &last_move);
	bool isGameOver(GameRules rules, const matrix<Sign> &board);
	bool isGameOver(GameRules rules, const matrix<Sign> &board, const Move &last_move);

	bool isDraw(GameRules rules, const matrix<Sign> &board);

}

#endif /* ALPHAGOMOKU_UTILS_GAME_RULES_HPP_ */
