/*
 * test_caro.cpp
 *
 *  Created on: Jun 30, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestCaro5, WinIn1)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" ! _ _ X _ _ ! _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" X _ _ X _ _ _ X _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ X _ _ _ _ X _ _ _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ ! _ _ _ _ _ X _ _ _ _ _\n" /*  3 */
					/*  4 */" X _ _ X _ _ X _ _ _ X _ _ _ _\n" /*  4 */
					/*  5 */" ! _ _ _ _ _ _ X _ _ _ ! _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ ! _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ X _ ! _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  8 */
					/*  9 */" _ X X X ! X _ _ _ X _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ !\n" /* 10 */
					/* 11 */" ! X X X X ! _ X _ _ _ _ _ X _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ ! _ _ _ _ _ X _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa5")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xd3")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa11")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xf11")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xe9")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xg0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xl5")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xi6")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xl7")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xg12")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xo10")), GameOutcome::CROSS_WIN);
	}
	TEST(TestCaro5, Overline)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa5")), GameOutcome::UNKNOWN);
	}
	TEST(TestCaro5, Blocked)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  3 */
					/*  4 */" ! _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  4 */
					/*  5 */" X _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" O _ _ _ _ _ X _ _ _ _ _ _ _ X\n" /*  6 */
					/*  7 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ X\n" /*  7 */
					/*  8 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ !\n" /*  8 */
					/*  9 */" _ _ _ O _ _ _ _ _ _ _ _ _ _ X\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xa4")), GameOutcome::UNKNOWN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xh5")), GameOutcome::UNKNOWN);
		EXPECT_EQ(getOutcome(GameRules::CARO5, board, Move("Xo8")), GameOutcome::UNKNOWN);
	}

	TEST(TestCaro6, WinIn1)
	{
// @formatter:off
			matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o          */
						/*  0 */" ! _ _ X _ _ ! _ _ _ _ _ _ _ _\n" /*  0 */
						/*  1 */" X _ _ X _ _ _ X _ _ _ _ _ _ _\n" /*  1 */
						/*  2 */" X _ _ X _ _ _ _ X _ _ _ _ _ _\n" /*  2 */
						/*  3 */" X _ _ ! _ _ _ _ _ X _ _ _ _ _\n" /*  3 */
						/*  4 */" X _ _ X _ _ X _ _ _ X _ _ _ _\n" /*  4 */
						/*  5 */" ! _ _ _ _ _ _ X _ _ _ ! _ _ _\n" /*  5 */
						/*  6 */" _ _ _ _ _ _ _ _ ! _ _ _ _ _ _\n" /*  6 */
						/*  7 */" _ _ _ _ _ _ _ _ _ X _ ! _ _ _\n" /*  7 */
						/*  8 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  8 */
						/*  9 */" _ X X X ! X _ _ _ X _ _ _ _ _\n" /*  9 */
						/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ !\n" /* 10 */
						/* 11 */" ! X X X X ! _ X _ _ _ _ _ X _\n" /* 11 */
						/* 12 */" _ _ _ _ _ _ ! _ _ _ _ _ X _ _\n" /* 12 */
						/* 13 */" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n" /* 13 */
						/* 14 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /* 14 */
						/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa5")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xd3")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa11")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xf11")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xe9")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xg0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xl5")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xi6")), GameOutcome::CROSS_WIN);

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xl7")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xg12")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xo10")), GameOutcome::CROSS_WIN);
	}
	TEST(TestCaro6, Overline)
	{
// @formatter:off
			matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o          */
						/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
						/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
						/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
						/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
						/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
						/*  5 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
						/*  6 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
						/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
						/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
						/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
						/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
						/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
						/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
						/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
						/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
						/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa0")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa5")), GameOutcome::CROSS_WIN);
	}
	TEST(TestCaro6, Blocked)
	{
// @formatter:off
			matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o          */
						/*  0 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
						/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
						/*  2 */" X _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /*  2 */
						/*  3 */" X _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  3 */
						/*  4 */" ! _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  4 */
						/*  5 */" X _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  5 */
						/*  6 */" O _ _ _ _ _ X _ _ _ _ _ _ _ X\n" /*  6 */
						/*  7 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ X\n" /*  7 */
						/*  8 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ !\n" /*  8 */
						/*  9 */" _ _ _ O _ _ _ _ _ _ _ _ _ _ X\n" /*  9 */
						/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /* 10 */
						/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /* 11 */
						/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O\n" /* 12 */
						/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
						/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
						/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xa4")), GameOutcome::UNKNOWN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xh5")), GameOutcome::CROSS_WIN);
		EXPECT_EQ(getOutcome(GameRules::CARO6, board, Move("Xo8")), GameOutcome::CROSS_WIN);
	}

//	TEST(TestCaro, Placeholder)
//	{
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//
//		EXPECT_TRUE(true);
//	}
} /* namespace ag */
