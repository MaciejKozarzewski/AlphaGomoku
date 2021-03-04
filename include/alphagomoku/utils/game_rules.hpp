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
	const int ROW_TO_WIN = 5;

	enum GameRules
	{
		FREESTYLE, STANDARD, RENJU, CARO
	};

	Sign getWhoWins(GameRules rules, const matrix<Sign> &board);
	Sign getWhoWins(GameRules rules, const matrix<Sign> &board, const Move &last_move);
	bool isGameOver(GameRules rules, const matrix<Sign> &board);
	bool isGameOver(GameRules rules, const matrix<Sign> &board, const Move &last_move);

	bool isDraw(GameRules rules, const matrix<Sign> &board);

	//functions for freestyle rules
	Sign getWhoWinsFreestyle(const matrix<Sign> &board);
	Sign getWhoWinsFreestyle(const matrix<Sign> &board, const Move &last_move);
	bool isDrawFreestyle(const matrix<Sign> &board);

	//functions for standard rules
	Sign getWhoWinsStandard(const matrix<Sign> &board);
	Sign getWhoWinsStandard(const matrix<Sign> &board, const Move &last_move);
	bool isDrawStandard(const matrix<Sign> &board);

	//functions for renju rules
	Sign getWhoWinsRenju(const matrix<Sign> &board);
	Sign getWhoWinsRenju(const matrix<Sign> &board, const Move &last_move);
	bool isDrawRenju(const matrix<Sign> &board);

	//functions for caro rules
	Sign getWhoWinsCaro(const matrix<Sign> &board);
	Sign getWhoWinsCaro(const matrix<Sign> &board, const Move &last_move);
	bool isDrawCaro(const matrix<Sign> &board);

}

#endif /* ALPHAGOMOKU_UTILS_GAME_RULES_HPP_ */
