/*
 * augmentations.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/augmentations.hpp>

namespace ag
{
	Move augment(Move move, int height, int width, int mode) noexcept
	{
		if (mode == 0)
			return move;
		if (height == width)
			assert(-8 < mode && mode < 8);
		else
			assert(-4 < mode && mode < 4);

		switch (mode)
		{
			case 1:
			case -1:
				return Move(move.sign, height - 1 - move.row, move.col); // reflect x
			case 2:
			case -2:
				return Move(move.sign, height - 1 - move.row, width - 1 - move.col); // reflect y
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

		// reflect x
//		if (mode == 1 || mode == -1)
//			return Move(height - 1 - move.row, move.col, move.sign);
		// reflect y
//		if (mode == 2 || mode == -2)
//			return Move(move.row, width - 1 - move.col, move.sign);
		// rotate 180
//		if (mode == 3 || mode == -3)
//			return Move(height - 1 - move.row, width - 1 - move.col, move.sign);

		// reflect diagonal
//		if (mode == 4 || mode == -4)
//			return Move(move.col, move.row, move.sign);
		// reflect antidiagonal
//		if (mode == 5 || mode == -5)
//			return Move(height - 1 - move.col, height - 1 - move.row, move.sign);

		// rotate 90
//		if (mode == 6 || mode == -7)
//			return Move(move.col, height - 1 - move.row, move.sign);
		// rotate 270
//		if (mode == 7 || mode == -6)
//			return Move(height - 1 - move.col, move.row, move.sign);

//		return move;
	}
}
