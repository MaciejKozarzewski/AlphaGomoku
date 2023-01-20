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
	Move augment(Move move, int height, int width, int mode) noexcept
	{
		assert(-available_symmetries(height, width) < mode && mode < available_symmetries(height, width));
		if (mode == 0)
			return move;

		switch (mode)
		{
			case 1:
			case -1:
				return Move(move.sign, height - 1 - move.row, move.col); // reflect x
			case 2:
			case -2:
				return Move(move.sign, move.row, width - 1 - move.col); // reflect y
			case 3:
			case -3:
				return Move(move.sign, height - 1 - move.row, width - 1 - move.col); // rotate 180 degrees
			case 4:
			case -4:
				return Move(move.sign, move.col, move.row); // reflect diagonal
			case 5:
			case -5:
				return Move(move.sign, height - 1 - move.col, height - 1 - move.row); // reflect antidiagonal
			case 6:
			case -7:
				return Move(move.sign, move.col, height - 1 - move.row); // rotate 90 degrees
			case 7:
			case -6:
				return Move(move.sign, height - 1 - move.col, move.row); // rotate 270 degrees
			default:
				return move;
		}
	}
}
