/*
 * game_rules.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/game_rules.hpp>
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
				return isDrawFreestyle(board);
			case GameRules::STANDARD:
				return isDrawStandard(board);
			case GameRules::RENJU:
				return isDrawRenju(board);
			case GameRules::CARO:
				return isDrawCaro(board);
		}
	}

//functions for freestyle rules
	Sign getWhoWinsFreestyle(const matrix<Sign> &board)
	{
//#define CHECK_BOARD_SPOT(r, c)\
//	switch (board.at(r, c))\
//	{\
//		case Sign::NONE:\
//			cross = 0;\
//			circle = 0;\
//			break;\
//		case Sign::CROSS:\
//			cross++;\
//			circle = 0;\
//			break;\
//		case Sign::CIRCLE:\
//			cross = 0;\
//			circle++;\
//			break;\
//	}\
//	if(cross >= ROW_TO_WIN)\
//		return Sign::CROSS;\
//	if(circle >= ROW_TO_WIN)\
//		return Sign::CIRCLE;
//
//		// horizontal lines
//		for (int row = 0; row < board.rows(); row++)
//		{
//			int cross = 0, circle = 0;
//			for (int col = 0; col < board.cols(); col++)
//			{
//				CHECK_BOARD_SPOT(row, col)
//			}
//		}
//		// vertical lines
//		for (int row = 0; row < board.rows(); row++)
//		{
//			int cross = 0, circle = 0;
//			for (int col = 0; col < board.cols(); col++)
//			{
//				CHECK_BOARD_SPOT(col, row)
//			}
//		}
//		// upper diagonal
//		for (int row = 0; row < board.rows(); row++)
//		{
//			int cross = 0, circle = 0;
//			for (int i = 0; i < board.cols() - row; i++)
//			{
//				CHECK_BOARD_SPOT(row + i, 0)
//			}
//		}
//
//		for (int j = 0; j < size; j++)
//		{
//			krzyzyk = 0;
//			kolko = 0;
//			for (int i = 0; i < size - j; i++)
//			{
//				switch (board.at(i, j + i))
//				{
//					case 1:
//						krzyzyk++;
//						kolko = 0;
//						break;
//					case 0:
//						krzyzyk = 0;
//						kolko = 0;
//						break;
//					case -1:
//						krzyzyk = 0;
//						kolko++;
//						break;
//				}
//				if (krzyzyk >= ROW_TO_WIN)
//					return 1;
//				if (kolko >= ROW_TO_WIN)
//					return -1;
//			}
//			krzyzyk = 0;
//			kolko = 0;
//			for (int i = 0; i < size - j; i++)
//			{
//				switch (board.at(j + i, i))
//				{
//					case 1:
//						krzyzyk++;
//						kolko = 0;
//						break;
//					case 0:
//						krzyzyk = 0;
//						kolko = 0;
//						break;
//					case -1:
//						krzyzyk = 0;
//						kolko++;
//						break;
//				}
//				if (krzyzyk >= ROW_TO_WIN)
//					return 1;
//				if (kolko >= ROW_TO_WIN)
//					return -1;
//			}
//			krzyzyk = 0;
//			kolko = 0;
//			for (int i = 0; i < size - j; i++)
//			{
//				switch (board.at(j + i, size - 1 - i))
//				{
//					case 1:
//						krzyzyk++;
//						kolko = 0;
//						break;
//					case 0:
//						krzyzyk = 0;
//						kolko = 0;
//						break;
//					case -1:
//						krzyzyk = 0;
//						kolko++;
//						break;
//				}
//				if (krzyzyk >= ROW_TO_WIN)
//					return 1;
//				if (kolko >= ROW_TO_WIN)
//					return -1;
//			}
//			krzyzyk = 0;
//			kolko = 0;
//			for (int i = 0; i < size - j; i++)
//			{
//				switch (board.at(size - 1 - j - i, i))
//				{
//					case 1:
//						krzyzyk++;
//						kolko = 0;
//						break;
//					case 0:
//						krzyzyk = 0;
//						kolko = 0;
//						break;
//					case -1:
//						krzyzyk = 0;
//						kolko++;
//						break;
//				}
//				if (krzyzyk >= ROW_TO_WIN)
//					return 1;
//				if (kolko >= ROW_TO_WIN)
//					return -1;
//			}
//		}
//		return 0;
	}
	Sign getWhoWinsFreestyle(const matrix<Sign> &board, const Move &last_move)
	{
	}
	bool isDrawFreestyle(const matrix<Sign> &board)
	{
	}

//functions for standard rules
	Sign getWhoWinsStandard(const matrix<Sign> &board)
	{
	}
	Sign getWhoWinsStandard(const matrix<Sign> &board, const Move &last_move)
	{
	}
	bool isDrawStandard(const matrix<Sign> &board)
	{
	}

//functions for renju rules
	Sign getWhoWinsRenju(const matrix<Sign> &board)
	{
	}
	Sign getWhoWinsRenju(const matrix<Sign> &board, const Move &last_move)
	{
	}
	bool isDrawRenju(const matrix<Sign> &board)
	{
	}

//functions for caro rules
	Sign getWhoWinsCaro(const matrix<Sign> &board)
	{
	}
	Sign getWhoWinsCaro(const matrix<Sign> &board, const Move &last_move)
	{
	}
	bool isDrawCaro(const matrix<Sign> &board)
	{
	}
}

