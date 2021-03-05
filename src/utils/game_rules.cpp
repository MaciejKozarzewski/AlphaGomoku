/*
 * game_rules.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/rules/freestyle.hpp>
#include <alphagomoku/rules/standard.hpp>
#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/caro.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <stdexcept>

namespace ag
{

	Sign getWhoWins(GameRules rules, const matrix<Sign> &board)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return getWhoWinsFreestyle(board);
			case GameRules::STANDARD:
				return getWhoWinsStandard(board);
			case GameRules::RENJU:
				return getWhoWinsRenju(board);
			case GameRules::CARO:
				return getWhoWinsCaro(board);
		}
	}
	Sign getWhoWins(GameRules rules, const matrix<Sign> &board, const Move &last_move)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return getWhoWinsFreestyle(board, last_move);
			case GameRules::STANDARD:
				return getWhoWinsStandard(board, last_move);
			case GameRules::RENJU:
				return getWhoWinsRenju(board, last_move);
			case GameRules::CARO:
				return getWhoWinsCaro(board, last_move);
		}
	}
	bool isGameOver(GameRules rules, const matrix<Sign> &board)
	{
		if (getWhoWins(rules, board) == Sign::NONE)
			return isBoardFull(board);
		else
			return true;
	}
	bool isGameOver(GameRules rules, const matrix<Sign> &board, const Move &last_move)
	{
		if (getWhoWins(rules, board, last_move) == Sign::NONE)
			return isBoardFull(board);
		else
			return true;
	}

	bool isDraw(GameRules rules, const matrix<Sign> &board)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return isBoardFull(board);
			case GameRules::STANDARD:
				return isBoardFull(board);
			case GameRules::RENJU:
				return isBoardFull(board);
			case GameRules::CARO:
				return isBoardFull(board);
		}
	}

}

