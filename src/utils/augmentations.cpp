/*
 * augmentations.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/augmentations.hpp>

#include <cassert>

namespace ag
{

	Move apply_symmetry(Move move, MatrixShape shape, Symmetry s) noexcept
	{
		assert(is_symmetry_allowed(s, shape));

		const int last_row = shape.rows - 1;
		const int last_col = shape.cols - 1;

		switch (s)
		{
			default:
			case Symmetry::IDENTITY:
				return move;
			case Symmetry::FLIP_VERTICALLY:
				return Move(move.sign, last_row - move.row, move.col);
			case Symmetry::FLIP_HORIZONTALLY:
				return Move(move.sign, move.row, last_col - move.col);
			case Symmetry::ROTATE_180:
				return Move(move.sign, last_row - move.row, last_col - move.col);
			case Symmetry::FLIP_DIAGONALLY:
				return Move(move.sign, move.col, move.row);
			case Symmetry::FLIP_ANTIDIAGONALLY:
				return Move(move.sign, last_col - move.col, last_row - move.row);
			case Symmetry::ROTATE_90:
				return Move(move.sign, move.col, last_row - move.row);
			case Symmetry::ROTATE_270:
				return Move(move.sign, last_col - move.col, move.row);
		}
	}

} /* namespace ag */
