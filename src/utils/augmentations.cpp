/*
 * augmentations.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/augmentations.hpp>

namespace ag
{
	Move augment(const Move &move, int height, int width, int mode)
	{
		if (mode == 0)
			return move;
		if (height == width)
			assert(mode < 8 && mode > -8);
		else
			assert(mode < 4 && mode > -4);

		// reflect x
		if (mode == 1 || mode == -1)
			return Move(height - 1 - move.row, move.col, move.sign);
		// reflect y
		if (mode == 2 || mode == -2)
			return Move(move.row, width - 1 - move.col, move.sign);
		// rotate 180
		if (mode == 3 || mode == -3)
			return Move(height - 1 - move.row, width - 1 - move.col, move.sign);

		// reflect diagonal
		if (mode == 4 || mode == -4)
			return Move(move.col, move.row, move.sign);
		// reflect antidiagonal
		if (mode == 5 || mode == -5)
			return Move(height - 1 - move.col, height - 1 - move.row, move.sign);

		// rotate 90
		if (mode == 6 || mode == -7)
			return Move(move.col, height - 1 - move.row, move.sign);
		// rotate 270
		if (mode == 7 || mode == -6)
			return Move(height - 1 - move.col, move.row, move.sign);

		return move;
	}
}
