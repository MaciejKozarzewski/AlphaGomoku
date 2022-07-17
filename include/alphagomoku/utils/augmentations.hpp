/*
 * augmentations.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_AUGMENTATIONS_HPP_
#define ALPHAGOMOKU_UTILS_AUGMENTATIONS_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>

namespace ag
{
	template<typename T>
	int available_symmetries(const matrix<T> &input) noexcept
	{
		if (input.isSquare())
			return 8;
		else
			return 4;
	}

	template<typename T>
	void augment(matrix<T> &input, int mode) noexcept
	{
		if (mode == 0)
			return;
		if (input.isSquare())
			assert(-8 < mode && mode < 8);
		else
			assert(-4 < mode && mode < 4);

		const int height = input.rows();
		const int width = input.cols();
		const int height1 = height - 1;

		switch (mode)
		{
			case 1: // reflect x
			case -1:
			{
				for (int i = 0; i < height / 2; i++)
					std::swap_ranges(input.data(i), input.data(i) + width, input.data(height1 - i));
				break;
			}
			case 2: // reflect y
			case -2:
			{
				for (int i = 0; i < height; i++)
					std::reverse(input.data(i), input.data(i) + width);
				break;
			}
			case 3: // rotate 180 degrees
			case -3:
			{
				std::reverse(input.begin(), input.end());
				break;
			}
			case 4: // reflect diagonal
			case -4:
			{
				for (int i = 0; i < height; i++)
				{
					T *ptr = input.data(i);
					for (int j = i + 1; j < height; j++)
						std::swap(ptr[j], input.at(j, i));
				}
				break;
			}
			case 5: // reflect antidiagonal
			case -5:
			{
				for (int i = 0; i < height; i++)
				{
					T *ptr = input.data(i);
					for (int j = 0; j < height1 - i; j++)
						std::swap(ptr[j], input.at(height1 - j, height1 - i));
				}
				break;
			}
			case 6: // rotate 90 degrees
			case -7:
			{
				for (int i = 0; i < height / 2; i++)
					for (int j = i; j < height1 - i; j++)
					{
						T tmp = input.at(i, j);
						input.at(i, j) = input.at(j, height1 - i);
						input.at(j, height1 - i) = input.at(height1 - i, height1 - j);
						input.at(height1 - i, height1 - j) = input.at(height1 - j, i);
						input.at(height1 - j, i) = tmp;
					}
				break;
			}
			case 7: // rotate 270 degrees
			case -6:
			{
				for (int i = 0; i < height / 2; i++)
					for (int j = i; j < height1 - i; j++)
					{
						T tmp = input.at(i, j);
						input.at(i, j) = input.at(height1 - j, i);
						input.at(height1 - j, i) = input.at(height1 - i, height1 - j);
						input.at(height1 - i, height1 - j) = input.at(j, height1 - i);
						input.at(j, height1 - i) = tmp;
					}
				break;
			}
		}
	}

	template<typename T>
	void augment(matrix<T> &dst, const matrix<T> &src, int mode) noexcept
	{
		assert(&dst != &src); // will not work 'in-place'
		assert(dst.rows() == src.rows() && dst.cols() == src.cols());
		if (mode == 0)
		{
			dst.copyFrom(src);
			return;
		}
		if (dst.isSquare())
			assert(-8 < mode && mode < 8);
		else
			assert(-4 < mode && mode < 4);

		const int height = src.rows();
		const int width = src.cols();
		const int height1 = height - 1;

		// reflect x
		if (mode == 1 || mode == -1)
		{
			for (int i = 0; i < height; i++)
				std::copy(src.data(i), src.data(i) + width, dst.data(height1 - i));
			return;
		}
		// reflect y
		if (mode == 2 || mode == -2)
		{
			for (int i = 0; i < height; i++)
				std::reverse_copy(src.data(i), src.data(i) + width, dst.data(i));
			return;
		}
		// rotate 180
		if (mode == 3 || mode == -3)
		{
			std::reverse_copy(src.begin(), src.end(), dst.begin());
			return;
		}

		// reflect diagonal
		if (mode == 4 || mode == -4)
		{
			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					dst.at(i, j) = src.at(j, i);
			return;
		}
		// reflect antidiagonal
		if (mode == 5 || mode == -5)
		{
			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					dst.at(i, j) = src.at(height1 - j, height1 - i);
			return;
		}

		// rotate 90 to the left
		if (mode == 6 || mode == -7)
		{
			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					dst.at(i, j) = src.at(j, height1 - i);
			return;
		}
		// rotate 270 to the left
		if (mode == 7 || mode == -6)
		{
			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					dst.at(i, j) = src.at(height1 - j, i);
			return;
		}
	}

	Move augment(Move move, int height, int width, int mode) noexcept;

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_AUGMENTATIONS_HPP_ */
