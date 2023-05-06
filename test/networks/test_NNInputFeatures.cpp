/*
 * test_NNInputFeatures.cpp
 *
 *  Created on: Feb 26, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/NNInputFeatures.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <gtest/gtest.h>

namespace
{
	template<int N>
	bool is_set_bit(uint32_t x) noexcept
	{
		return (x & (1u << N)) != 0;
	}
}

namespace ag
{

	TEST(TestNNInputFeatures, is_set_bit)
	{
		const uint32_t value = (1u << 10u) | (1u << 1u) | (1u << 31u);

		EXPECT_TRUE(is_set_bit<1>(value));
		EXPECT_TRUE(is_set_bit<10>(value));
		EXPECT_TRUE(is_set_bit<31>(value));
		EXPECT_FALSE(is_set_bit<0>(value));
		EXPECT_FALSE(is_set_bit<2>(value));
	}
	TEST(TestNNInputFeatures, augmentation)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l m n o          */
				/*  0 */ " _ _ O O _ _ _ _ _ X _ X X X X\n" /*  0 */
				/*  1 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " O _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " O _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ _ O O O O _ _ _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ O _ _ _ _ _ _ _ _ _ O _ _\n" /*  8 */
				/*  9 */ " _ _ _ O _ _ _ _ _ _ _ O _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/* 12 */ " X _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /* 12 */
				/* 13 */ " X _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */ " X _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /* 14 */
				/*         a b c d e f g h i j k l m n o          */);
// @formatter:on
		const Sign sign_to_move = Sign::CROSS;

		const GameConfig cfg(GameRules::STANDARD, board.rows(), board.cols());

		PatternCalculator calc(cfg);
		NNInputFeatures correct(cfg.rows, cfg.cols);
		NNInputFeatures features(cfg.rows, cfg.cols);

		matrix<Sign> tmp(board.rows(), board.cols());
		for (int mode = -7; mode <= 7; mode++)
		{
			calc.setBoard(board, sign_to_move);
			features.encode(calc);
			features.augment(mode);

			augment(tmp, board, mode);
			calc.setBoard(tmp, sign_to_move);
			correct.encode(calc);

			EXPECT_EQ(correct, features);
		}
	}

	TEST(TestNNInputFeatures, board1_circle_to_move)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l          */
				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " _ _ _ _ _ O O _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ X O _ X _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ O X _ X _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/*         a b c d e f g h i j k l          */);
// @formatter:on
		const Sign sign_to_move = Sign::CIRCLE;

		const GameConfig cfg(GameRules::STANDARD, board.rows(), board.cols());

		PatternCalculator calc(cfg);
		calc.setBoard(board, sign_to_move);

		NNInputFeatures features(cfg.rows, cfg.cols);
		features.encode(calc);

		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
			{
				const uint32_t x = features.at(row, col);
				switch (board.at(row, col))
				{
					case Sign::NONE:
						EXPECT_TRUE(is_set_bit<0>(x));  // legal move
						EXPECT_FALSE(is_set_bit<1>(x)); // own stone
						EXPECT_FALSE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::CROSS:
						EXPECT_FALSE(is_set_bit<0>(x));  // legal move
						EXPECT_FALSE(is_set_bit<1>(x)); // own stone
						EXPECT_TRUE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::CIRCLE:
						EXPECT_FALSE(is_set_bit<0>(x));  // legal move
						EXPECT_TRUE(is_set_bit<1>(x)); // own stone
						EXPECT_FALSE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::ILLEGAL:
						break;
				}
				EXPECT_TRUE(is_set_bit<3>(x));  // ones (constant channel)
				EXPECT_FALSE(is_set_bit<4>(x)); // black (cross) to move
				EXPECT_TRUE(is_set_bit<5>(x));  // white (circle) to move
				EXPECT_FALSE(is_set_bit<6>(x)); // forbidden move
				EXPECT_FALSE(is_set_bit<7>(x)); // zeros (constant channel)
			}

		// cross features
		EXPECT_TRUE(is_set_bit<22>(features.at(3, 2)));
		EXPECT_TRUE(is_set_bit<22>(features.at(4, 3)));
		EXPECT_TRUE(is_set_bit<23>(features.at(7, 2)));
		EXPECT_TRUE(is_set_bit<22>(features.at(7, 6)));
		EXPECT_TRUE(is_set_bit<23>(features.at(8, 1)));
		EXPECT_TRUE(is_set_bit<22>(features.at(8, 7)));

		// circle features
		EXPECT_TRUE(is_set_bit<8>(features.at(3, 3)));
		EXPECT_TRUE(is_set_bit<8>(features.at(3, 4)));
		EXPECT_TRUE(is_set_bit<8>(features.at(3, 7)));
		EXPECT_TRUE(is_set_bit<8>(features.at(3, 8)));
		EXPECT_TRUE(is_set_bit<11>(features.at(4, 4)));
		EXPECT_TRUE(is_set_bit<11>(features.at(5, 3)));
	}
	TEST(TestNNInputFeatures, board1_cross_to_move)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l          */
				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " _ _ _ _ _ O O _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ X O _ X _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ O X _ X _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/*         a b c d e f g h i j k l          */);
// @formatter:on
		const Sign sign_to_move = Sign::CROSS;

		const GameConfig cfg(GameRules::STANDARD, board.rows(), board.cols());

		PatternCalculator calc(cfg);
		calc.setBoard(board, sign_to_move);

		NNInputFeatures features(cfg.rows, cfg.cols);
		features.encode(calc);

		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
			{
				const uint32_t x = features.at(row, col);
				switch (board.at(row, col))
				{
					case Sign::NONE:
						EXPECT_TRUE(is_set_bit<0>(x));  // legal move
						EXPECT_FALSE(is_set_bit<1>(x)); // own stone
						EXPECT_FALSE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::CROSS:
						EXPECT_FALSE(is_set_bit<0>(x));  // legal move
						EXPECT_TRUE(is_set_bit<1>(x)); // own stone
						EXPECT_FALSE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::CIRCLE:
						EXPECT_FALSE(is_set_bit<0>(x));  // legal move
						EXPECT_FALSE(is_set_bit<1>(x)); // own stone
						EXPECT_TRUE(is_set_bit<2>(x)); // opponent stone
						break;
					case Sign::ILLEGAL:
						break;
				}
				EXPECT_TRUE(is_set_bit<3>(x));  // ones (constant channel)
				EXPECT_TRUE(is_set_bit<4>(x)); // black (cross) to move
				EXPECT_FALSE(is_set_bit<5>(x));  // white (circle) to move
				EXPECT_FALSE(is_set_bit<6>(x)); // forbidden move
				EXPECT_FALSE(is_set_bit<7>(x)); // zeros (constant channel)
			}

		// cross features
		EXPECT_TRUE(is_set_bit<10>(features.at(3, 2)));
		EXPECT_TRUE(is_set_bit<10>(features.at(4, 3)));
		EXPECT_TRUE(is_set_bit<11>(features.at(7, 2)));
		EXPECT_TRUE(is_set_bit<10>(features.at(7, 6)));
		EXPECT_TRUE(is_set_bit<11>(features.at(8, 1)));
		EXPECT_TRUE(is_set_bit<10>(features.at(8, 7)));

		// circle features
		EXPECT_TRUE(is_set_bit<20>(features.at(3, 3)));
		EXPECT_TRUE(is_set_bit<20>(features.at(3, 4)));
		EXPECT_TRUE(is_set_bit<20>(features.at(3, 7)));
		EXPECT_TRUE(is_set_bit<20>(features.at(3, 8)));
		EXPECT_TRUE(is_set_bit<23>(features.at(4, 4)));
		EXPECT_TRUE(is_set_bit<23>(features.at(5, 3)));
	}

	TEST(TestNNInputFeatures, board2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l          */
				/*  0 */ " _ _ O O _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " O _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " O _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ O _ _ _ _ _ _ O _ _\n" /*  8 */
				/*  9 */ " _ _ _ O _ _ _ _ O _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/*         a b c d e f g h i j k l          */);
// @formatter:on
		const Sign sign_to_move = Sign::CROSS;

		const GameConfig cfg(GameRules::STANDARD, board.rows(), board.cols());

		PatternCalculator calc(cfg);
		calc.setBoard(board, sign_to_move);

		NNInputFeatures features(cfg.rows, cfg.cols);
		features.encode(calc);

		// circle features
		EXPECT_TRUE(is_set_bit<20>(features.at(0, 1)));
		EXPECT_TRUE(is_set_bit<20>(features.at(0, 4)));
		EXPECT_TRUE(is_set_bit<20>(features.at(0, 5)));

		EXPECT_TRUE(is_set_bit<21>(features.at(1, 0)));
		EXPECT_TRUE(is_set_bit<21>(features.at(4, 0)));
		EXPECT_TRUE(is_set_bit<21>(features.at(5, 0)));

		EXPECT_TRUE(is_set_bit<22>(features.at(7, 1)));
		EXPECT_TRUE(is_set_bit<22>(features.at(10, 4)));

		EXPECT_TRUE(is_set_bit<23>(features.at(7, 10)));
		EXPECT_TRUE(is_set_bit<23>(features.at(10, 7)));
	}

} /* namespace ag */

