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

namespace ag
{
	enum class Symmetry
	{
		IDENTITY,
		FLIP_VERTICALLY,
		FLIP_HORIZONTALLY,
		ROTATE_180,
		FLIP_DIAGONALLY,
		FLIP_ANTIDIAGONALLY,
		ROTATE_90,
		ROTATE_270
	};

	inline Symmetry get_inverse_symmetry(Symmetry s) noexcept
	{
		switch (s)
		{
			default:
			case Symmetry::IDENTITY:
				return Symmetry::IDENTITY;
			case Symmetry::FLIP_VERTICALLY:
				return Symmetry::FLIP_VERTICALLY;
			case Symmetry::FLIP_HORIZONTALLY:
				return Symmetry::FLIP_HORIZONTALLY;
			case Symmetry::ROTATE_180:
				return Symmetry::ROTATE_180;
			case Symmetry::FLIP_DIAGONALLY:
				return Symmetry::FLIP_DIAGONALLY;
			case Symmetry::FLIP_ANTIDIAGONALLY:
				return Symmetry::FLIP_ANTIDIAGONALLY;
			case Symmetry::ROTATE_90:
				return Symmetry::ROTATE_270;
			case Symmetry::ROTATE_270:
				return Symmetry::ROTATE_90;
		}
	}

	inline Symmetry int_to_symmetry(int i) noexcept
	{
		assert(0 <= i && i < 8);
		return static_cast<Symmetry>(i);
	}

	inline int number_of_available_symmetries(MatrixShape shape) noexcept
	{
		return shape.isSquare() ? 8 : 4;
	}

	inline bool is_symmetry_allowed(Symmetry s, MatrixShape shape) noexcept
	{
		return static_cast<int>(s) < number_of_available_symmetries(shape);
	}

	template<typename T>
	void apply_symmetry_in_place(matrix<T> &input, Symmetry s) noexcept
	{
		assert(is_symmetry_allowed(s, input.shape()));

		const int rows = input.rows();
		const int cols = input.cols();
		const int last_idx = rows - 1; // used only when matrices are square (rows == cols)

		switch (s)
		{
			case Symmetry::IDENTITY:
				break;
			case Symmetry::FLIP_VERTICALLY:
			{
				for (int r = 0; r < rows / 2; r++)
					std::swap_ranges(input.data(r), input.data(r) + cols, input.data(last_idx - r));
				break;
			}
			case Symmetry::FLIP_HORIZONTALLY:
			{
				for (int r = 0; r < rows; r++)
					std::reverse(input.data(r), input.data(r) + cols);
				break;
			}
			case Symmetry::ROTATE_180:
			{
				std::reverse(input.begin(), input.end());
				break;
			}
			case Symmetry::FLIP_DIAGONALLY:
			{
				for (int r = 0; r < rows; r++)
				{
					T *ptr = input.data(r);
					for (int c = r + 1; c < rows; c++)
						std::swap(ptr[c], input.at(c, r));
				}
				break;
			}
			case Symmetry::FLIP_ANTIDIAGONALLY:
			{
				for (int r = 0; r < rows; r++)
				{
					T *ptr = input.data(r);
					for (int c = 0; c < last_idx - r; c++)
						std::swap(ptr[c], input.at(last_idx - c, last_idx - r));
				}
				break;
			}
			case Symmetry::ROTATE_90:
			{
				for (int r = 0; r < rows / 2; r++)
					for (int c = r; c < last_idx - r; c++)
					{
						const T tmp = input.at(r, c);
						input.at(r, c) = input.at(c, last_idx - r);
						input.at(c, last_idx - r) = input.at(last_idx - r, last_idx - c);
						input.at(last_idx - r, last_idx - c) = input.at(last_idx - c, r);
						input.at(last_idx - c, r) = tmp;
					}
				break;
			}
			case Symmetry::ROTATE_270:
			{
				for (int r = 0; r < rows / 2; r++)
					for (int c = r; c < last_idx - r; c++)
					{
						const T tmp = input.at(r, c);
						input.at(r, c) = input.at(last_idx - c, r);
						input.at(last_idx - c, r) = input.at(last_idx - r, last_idx - c);
						input.at(last_idx - r, last_idx - c) = input.at(c, last_idx - r);
						input.at(c, last_idx - r) = tmp;
					}
				break;
			}
		}
	}

	template<typename T>
	void apply_symmetry(matrix<T> &dst, const matrix<T> &src, Symmetry s) noexcept
	{
		assert(&dst != &src); // will not work 'in-place'
		assert(dst.shape() == src.shape());
		assert(is_symmetry_allowed(s, dst.shape()));

		const int rows = src.rows();
		const int cols = src.cols();
		const int last_idx = rows - 1; // used only when matrices are square (rows == cols)

		switch (s)
		{
			case Symmetry::IDENTITY:
			{
				dst.copyFrom(src);
				break;
			}
			case Symmetry::FLIP_VERTICALLY:
			{
				for (int r = 0; r < rows; r++)
					std::copy(src.data(r), src.data(r) + cols, dst.data(last_idx - r));
				break;
			}
			case Symmetry::FLIP_HORIZONTALLY:
			{
				for (int r = 0; r < rows; r++)
					std::reverse_copy(src.data(r), src.data(r) + cols, dst.data(r));
				break;
			}
			case Symmetry::ROTATE_180:
			{
				std::reverse_copy(src.begin(), src.end(), dst.begin());
				break;
			}
			case Symmetry::FLIP_DIAGONALLY:
			{
				for (int r = 0; r < rows; r++)
					for (int c = 0; c < cols; c++)
						dst.at(r, c) = src.at(c, r);
				break;
			}
			case Symmetry::FLIP_ANTIDIAGONALLY:
			{
				for (int r = 0; r < rows; r++)
					for (int c = 0; c < cols; c++)
						dst.at(r, c) = src.at(last_idx - c, last_idx - r);
				break;
			}
			case Symmetry::ROTATE_90:
			{
				for (int r = 0; r < rows; r++)
					for (int c = 0; c < cols; c++)
						dst.at(r, c) = src.at(c, last_idx - r);
				break;
			}
			case Symmetry::ROTATE_270:
			{
				for (int r = 0; r < rows; r++)
					for (int c = 0; c < cols; c++)
						dst.at(r, c) = src.at(last_idx - c, r);
				break;
			}
		}
	}

	Move apply_symmetry(Move move, MatrixShape shape, Symmetry s) noexcept;

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_AUGMENTATIONS_HPP_ */
