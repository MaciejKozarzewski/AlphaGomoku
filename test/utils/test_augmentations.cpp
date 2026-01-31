/*
 * test_augmentations.cpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/augmentations.hpp>

#include <gtest/gtest.h>

namespace
{
	ag::matrix<int> get_matrix(int rows, int cols)
	{
		assert(rows >= 8 && cols >= 8);
		ag::matrix<int> result(rows, cols);
		result.at(0, 0) = 1;
		result.at(3, 4) = 2;
		result.at(2, 7) = 3;
		result.at(7, 6) = 4;
		result.at(5, 1) = 5;
		result.at(4, 5) = 6;
		return result;
	}
	void print_matrix(const ag::matrix<int> &m)
	{
		for (int i = 0; i < m.rows(); i++)
		{
			for (int j = 0; j < m.cols(); j++)
				std::cout << m.at(i, j) << ' ';
			std::cout << '\n';
		}
		std::cout << "-----------------------------------\n";
	}
}

namespace ag
{

	TEST(TestAugmentations, none)
	{
		const Symmetry mode = Symmetry::IDENTITY;
		matrix<int> src = get_matrix(8, 9);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(0, 0), 1);
		EXPECT_EQ(dst.at(3, 4), 2);
		EXPECT_EQ(dst.at(2, 7), 3);
		EXPECT_EQ(dst.at(7, 6), 4);
		EXPECT_EQ(dst.at(5, 1), 5);
		EXPECT_EQ(dst.at(4, 5), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 9);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, flip_vertically)
	{
		const Symmetry mode = Symmetry::FLIP_VERTICALLY;
		matrix<int> src = get_matrix(8, 9);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(7, 0), 1);
		EXPECT_EQ(dst.at(4, 4), 2);
		EXPECT_EQ(dst.at(5, 7), 3);
		EXPECT_EQ(dst.at(0, 6), 4);
		EXPECT_EQ(dst.at(2, 1), 5);
		EXPECT_EQ(dst.at(3, 5), 6);

		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 9);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, flip_horizontally)
	{
		const Symmetry mode = Symmetry::FLIP_HORIZONTALLY;
		matrix<int> src = get_matrix(8, 9);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(0, 8), 1);
		EXPECT_EQ(dst.at(3, 4), 2);
		EXPECT_EQ(dst.at(2, 1), 3);
		EXPECT_EQ(dst.at(7, 2), 4);
		EXPECT_EQ(dst.at(5, 7), 5);
		EXPECT_EQ(dst.at(4, 3), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 9);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, rotate_180)
	{
		const Symmetry mode = Symmetry::ROTATE_180;
		matrix<int> src = get_matrix(8, 9);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(7, 8), 1);
		EXPECT_EQ(dst.at(4, 4), 2);
		EXPECT_EQ(dst.at(5, 1), 3);
		EXPECT_EQ(dst.at(0, 2), 4);
		EXPECT_EQ(dst.at(2, 7), 5);
		EXPECT_EQ(dst.at(3, 3), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 9);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}

	TEST(TestAugmentations, flip_diagonally)
	{
		const Symmetry mode = Symmetry::FLIP_DIAGONALLY;
		matrix<int> src = get_matrix(8, 8);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(0, 0), 1);
		EXPECT_EQ(dst.at(4, 3), 2);
		EXPECT_EQ(dst.at(7, 2), 3);
		EXPECT_EQ(dst.at(6, 7), 4);
		EXPECT_EQ(dst.at(1, 5), 5);
		EXPECT_EQ(dst.at(5, 4), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 8);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, flip_antidiagonally)
	{
		const Symmetry mode = Symmetry::FLIP_ANTIDIAGONALLY;
		matrix<int> src = get_matrix(8, 8);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(7, 7), 1);
		EXPECT_EQ(dst.at(3, 4), 2);
		EXPECT_EQ(dst.at(0, 5), 3);
		EXPECT_EQ(dst.at(1, 0), 4);
		EXPECT_EQ(dst.at(6, 2), 5);
		EXPECT_EQ(dst.at(2, 3), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 8);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, rotate_90)
	{
		const Symmetry mode = Symmetry::ROTATE_90;
		matrix<int> src = get_matrix(8, 8);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(7, 0), 1);
		EXPECT_EQ(dst.at(3, 3), 2);
		EXPECT_EQ(dst.at(0, 2), 3);
		EXPECT_EQ(dst.at(1, 7), 4);
		EXPECT_EQ(dst.at(6, 5), 5);
		EXPECT_EQ(dst.at(2, 4), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 8);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
	TEST(TestAugmentations, rotate_270)
	{
		const Symmetry mode = Symmetry::ROTATE_270;
		matrix<int> src = get_matrix(8, 8);
		matrix<int> dst(src.rows(), src.cols());

		apply_symmetry(dst, src, mode);
		EXPECT_EQ(dst.at(0, 7), 1);
		EXPECT_EQ(dst.at(4, 4), 2);
		EXPECT_EQ(dst.at(7, 5), 3);
		EXPECT_EQ(dst.at(6, 0), 4);
		EXPECT_EQ(dst.at(1, 2), 5);
		EXPECT_EQ(dst.at(5, 3), 6);
		matrix<int> inverted(src.rows(), src.cols());
		apply_symmetry(inverted, dst, get_inverse_symmetry(mode));
		EXPECT_EQ(inverted, src);

		matrix<int> in_place = get_matrix(8, 8);
		apply_symmetry_in_place(in_place, mode);
		EXPECT_EQ(in_place, dst);
		apply_symmetry_in_place(in_place, get_inverse_symmetry(mode));
		EXPECT_EQ(in_place, src);
	}
} /* namespace ag */

