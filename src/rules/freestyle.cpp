/*
 * freestyle.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include <alphagomoku/rules/freestyle.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	const int ROW_TO_WIN = 5;
}

namespace ag
{

	GameOutcome getOutcomeFreestyle(const matrix<Sign> &board)
	{
		assert(board.isSquare());

#define CHECK(x, y)\
	switch (board.at(x, y))\
	{\
		case Sign::NONE:\
			cross = 0, circle = 0;\
			break;\
		case Sign::CROSS:\
			cross++;\
			circle = 0;\
			break;\
		case Sign::CIRCLE:\
			cross = 0;\
			circle++;\
			break;\
	}\
	if (cross >= ROW_TO_WIN)\
		return GameOutcome::CROSS_WIN;\
	if (circle >= ROW_TO_WIN)\
		return GameOutcome::CIRCLE_WIN;

		int empty_spots = 0;
		const int size = board.rows();
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size; i++)
			{
				empty_spots += static_cast<int>(board.at(i, j) == Sign::NONE);
				CHECK(i, j)
			}
		}
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size; i++)
			{
				CHECK(j, i)
			}
		}
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size - j; i++)
			{
				CHECK(i, j + i)
			}
		}
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size - j; i++)
			{
				CHECK(j + i, i)
			}
		}
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size - j; i++)
			{
				CHECK(j + i, size - 1 - i)
			}
		}
		for (int j = 0; j < size; j++)
		{
			int cross = 0, circle = 0;
			for (int i = 0; i < size - j; i++)
			{
				CHECK(size - 1 - j - i, i)
			}
		}
#undef CHECK
		if (empty_spots == 0)
			return GameOutcome::DRAW;
		else
			return GameOutcome::UNKNOWN;
	}
	GameOutcome getOutcomeFreestyle(const matrix<Sign> &board, const Move &last_move)
	{
		assert(board.isSquare());
		assert(last_move.sign != Sign::NONE);
		assert(last_move.row >= 0 && last_move.row < board.rows());
		assert(last_move.col >= 0 && last_move.col < board.cols());
		const int x0 = std::max(last_move.row - ROW_TO_WIN + 1, 0);
		const int x1 = std::min(last_move.row + ROW_TO_WIN - 1, board.rows() - 1);
		const int y0 = std::max(last_move.col - ROW_TO_WIN + 1, 0);
		const int y1 = std::min(last_move.col + ROW_TO_WIN - 1, board.cols() - 1);

#define CHECK(x, y)\
	if (board.at(x, y) == last_move.sign)\
	{\
		tmp++;\
		if (tmp >= ROW_TO_WIN)\
			return static_cast<GameOutcome>(static_cast<int>(last_move.sign) + 1);\
	}\
	else\
		tmp = 0;

		int tmp = 0;
		for (int i = x0; i <= x1; i++)
			CHECK(i, last_move.col)

		tmp = 0;
		for (int i = y0; i <= y1; i++)
			CHECK(last_move.row, i)

		tmp = 0;
		int tmp0 = std::max(x0 - last_move.row, y0 - last_move.col);
		int tmp1 = std::min(x1 - last_move.row, y1 - last_move.col);
		for (int i = tmp0; i <= tmp1; i++)
			CHECK(last_move.row + i, last_move.col + i)

		tmp = 0;
		tmp0 = std::max(x0 - last_move.row, last_move.col - y1);
		tmp1 = std::min(x1 - last_move.row, last_move.col - y0);
		for (int i = tmp0; i <= tmp1; i++)
			CHECK(last_move.row + i, last_move.col - i)
#undef CHECK

		if (isBoardFull(board))
			return GameOutcome::DRAW;
		else
			return GameOutcome::UNKNOWN;
	}

	GameOutcome getOutcomeFreestyle(const std::vector<Sign> &line)
	{
		int cross = 0, circle = 0;
		for (size_t i = 0; i < line.size(); i++)
			switch (line[i])
			{
				default:
				case Sign::NONE:
					cross = 0, circle = 0;
					break;
				case Sign::CROSS:
					cross++;
					if (cross >= ROW_TO_WIN)
						return GameOutcome::CROSS_WIN;
					else
						circle = 0;
					break;
				case Sign::CIRCLE:
					cross = 0;
					if (circle >= ROW_TO_WIN)
						return GameOutcome::CIRCLE_WIN;
					else
						circle++;
					break;
			}
		return GameOutcome::UNKNOWN;
	}
}

