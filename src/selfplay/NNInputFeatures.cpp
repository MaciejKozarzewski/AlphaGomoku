/*
 * NNInputFeatures.cpp
 *
 *  Created on: Jan 19, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/NNInputFeatures.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/augmentations.hpp>

namespace
{
	using namespace ag;
	uint32_t encode_patterns(TwoPlayerGroup<DirectionGroup<PatternType>> patterns, Sign ownSign) noexcept
	{
		static const uint32_t table1[8] = { 0u, 0u, 1u, (1u << 4), 0u, 0u, 0u, 0u };
		static const uint32_t table2[8] = { 0u, 0u, 0u, 0u, (1u << 8u), (1u << 9u), (1u << 10u), (1u << 11u) };

		uint32_t result1 = 0, result2 = 0;
		for (uint32_t i = 0; i < 4u; i++)
		{
			const int idx1 = static_cast<int>(patterns.for_cross[i]);
			const int idx2 = static_cast<int>(patterns.for_circle[i]);
			result1 |= (table1[idx1] << i) | table2[idx1];
			result2 |= (table1[idx2] << i) | table2[idx2];
		}
		if (ownSign == Sign::CROSS)
			return (result1 << 8u) | (result2 << 20u);
		else
			return (result1 << 20u) | (result2 << 8u);
	}
	template<int D0, int D1, int D2, int D3>
	uint32_t shuffle_directions(const uint32_t data) noexcept
	{
		static_assert(D0 >= 0 && D0 < 4, "can only shuffle four bits");
		static_assert(D1 >= 0 && D1 < 4, "can only shuffle four bits");
		static_assert(D2 >= 0 && D2 < 4, "can only shuffle four bits");
		static_assert(D3 >= 0 && D3 < 4, "can only shuffle four bits");
		static_assert(D0 != D1 && D0 != D2 && D0 != D3 && D1 != D2 && D1 != D3 && D2 != D3, "each bit must be used exactly once");

		constexpr uint32_t mask = (1u << 8u) | (1u << 12u) | (1u << 20u) | (1u << 24u); // shuffling bits 8-11, 12-15, 20-23, 24-17

		uint32_t result = data & 0xF00F00FF; // 0xF00F00FF is a mask of all bits that are not shuffled
		result |= (((data >> D0) & mask) << 0);
		result |= (((data >> D1) & mask) << 1);
		result |= (((data >> D2) & mask) << 2);
		result |= (((data >> D3) & mask) << 3);
		return result;
	}
}

namespace ag
{
	NNInputFeatures::NNInputFeatures(int rows, int cols) :
			matrix<uint32_t>(rows, cols)
	{
	}
	void NNInputFeatures::encode(PatternCalculator &calc)
	{
		assert(rows() == calc.getConfig().rows);
		assert(cols() == calc.getConfig().cols);
		assert(calc.getSignToMove() != Sign::NONE);
		/*
		 * Input features are:
		 *  Bits	N	Description
		 *  0		1	legal move
		 *  1		1	own stone
		 *  2		1	opponent stone
		 *  3 		1	forbidden move
		 *  4 		1	black (cross) to move
		 *  5 		1	white (white) to move
		 *  6 		1	ones (constant channel)
		 *  7 		1	zeros (constant channel)
		 *
		 *   8-11	4	own open 3 (presence of a feature in each direction)
		 *  12-15	4	own half open 4
		 *  16		1	own open 4 (presence of a feature in any direction)
		 *  17		1	own double 4
		 *  18		1	own five
		 *  19		1	own overline
		 *
		 *  20-23	4	opponent open 3 (presence of a feature in each direction)
		 *  24-17	4	opponent half open 4
		 *  28		1	opponent open 4 (presence of a feature in any direction)
		 *  29		1	opponent double 4
		 *  30		1	opponent five
		 *  31		1	opponent overline
		 */
		const Sign own_sign = calc.getSignToMove();

		const uint32_t table[4] = { 1u, (own_sign == Sign::CROSS) ? 2u : 4u, (own_sign == Sign::CROSS) ? 4u : 2u, 0u };
		const uint32_t forbidden = 1u << 3u;
		const uint32_t color_to_move = (own_sign == Sign::CROSS) ? (1u << 4u) : (1u << 5u);
		const uint32_t ones = 1u << 6u;

		for (int row = 0; row < rows(); row++)
			for (int col = 0; col < cols(); col++)
			{
				uint32_t tmp = color_to_move | ones; // base value
				tmp |= table[static_cast<int>(calc.signAt(row, col))];
				tmp |= encode_patterns(calc.getPatternsAt(row, col), own_sign);
				this->at(row, col) = tmp;
			}
		if (calc.getConfig().rules == GameRules::RENJU and own_sign == Sign::CROSS)
			for (int row = 0; row < rows(); row++)
				for (int col = 0; col < cols(); col++)
					if (calc.isForbidden(own_sign, row, col))
						this->at(row, col) |= forbidden;
	}
	void NNInputFeatures::augment(int mode) noexcept
	{
		ag::augment(*this, mode);

		// now we have to shuffle bits 8-11, 12-15, 20-23, 24-17 because their values depend on directions
		switch (mode)
		{
			case 0:
				break;
			case 1: // reflect x
			case -1:
			case 2: // reflect y
			case -2:
			{ // horizontal and vertical stays the same, diagonals are swapped
				for (int i = 0; i < this->size(); i++)
					this->operator [](i) = shuffle_directions<0, 1, 3, 2>(this->operator [](i));
				break;
			}
			case 3: // rotate 180 degrees
			case -3:
				break; // nothing changes
			case 4: // reflect diagonal
			case -4:
			case 5: // reflect antidiagonal
			case -5:
			{ // horizontal and vertical are swapped, diagonals stays the same
				for (int i = 0; i < this->size(); i++)
					this->operator [](i) = shuffle_directions<1, 0, 2, 3>(this->operator [](i));
				break;
			}
			case 6: // rotate 90 degrees
			case -7:
			case 7: // rotate 270 degrees
			case -6:
			{ // horizontal and vertical are swapped, diagonals are swapped too
				for (int i = 0; i < this->size(); i++)
					this->operator [](i) = shuffle_directions<1, 0, 3, 2>(this->operator [](i));
				break;
			}
		}
	}

} /* namespace ag */

