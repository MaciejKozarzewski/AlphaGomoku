/*
 * test_renju.cpp
 *
 *  Created on: Aug 19, 2021
 *  	Author: Mira Fontan
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestRenju, WinIn1)
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
					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
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

		EXPECT_FALSE(isForbidden(board, Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_FALSE(isForbidden(board, Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CROSS_WIN);
	}
	TEST(TestRenju, Overline)
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

		EXPECT_FALSE(isForbidden(board, Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_TRUE(isForbidden(board, Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CIRCLE_WIN);
	}
	TEST(TestRenju, WinInsideOverline)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" ! X X X X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
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

		EXPECT_FALSE(isForbidden(board, Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_FALSE(isForbidden(board, Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CROSS_WIN);
	}
	TEST(TestRenju, FourAndFour)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ X ! _ X X _\n" /*  0 */
					/*  1 */" _ _ X ! X X O _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  2 */
					/*  3 */" _ _ _ X _ _ _ _ _ _ _ _ _ X _\n" /*  3 */
					/*  4 */" _ _ _ X _ _ O X X X ! _ _ _ X\n" /*  4 */
					/*  5 */" _ _ _ X _ _ _ _ _ _ X _ _ _ _\n" /*  5 */
					/*  6 */" X _ _ O _ _ _ _ _ _ X _ _ _ _\n" /*  6 */
					/*  7 */" X _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /*  8 */
					/*  9 */" ! _ _ _ _ X _ X ! X _ X _ _ _\n" /*  9 */
					/* 10 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" O _ _ _ _ X X X _ ! _ X X X _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xk0")));
		EXPECT_TRUE(isForbidden(board, Move("Xd1")));
		EXPECT_TRUE(isForbidden(board, Move("Xk4")));
		EXPECT_TRUE(isForbidden(board, Move("Xa9")));
		EXPECT_TRUE(isForbidden(board, Move("Xi9")));
		EXPECT_TRUE(isForbidden(board, Move("Xj14")));
	}
	TEST(TestRenju, FourAndFreeThree)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ ! _ _ _ _ O _ X ! X _ O\n" /*  1 */
					/*  2 */" _ _ X X _ _ _ _ _ _ _ X _ _ _\n" /*  2 */
					/*  3 */" _ X _ X _ _ _ _ _ _ _ X _ _ _\n" /*  3 */
					/*  4 */" _ _ _ X _ _ _ _ _ _ _ X _ _ _\n" /*  4 */
					/*  5 */" _ _ _ O _ _ _ _ _ _ _ O _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ X X ! _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" X _ _ _ _ _ _ O _ _ _ X _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /* 11 */
					/* 12 */" _ _ ! _ X X _ _ _ ! _ X X X O\n" /* 12 */
					/* 13 */" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xd1")));
		EXPECT_FALSE(isForbidden(board, Move("Xl1")));
		EXPECT_FALSE(isForbidden(board, Move("Xh6")));
		EXPECT_FALSE(isForbidden(board, Move("Xc12")));
		EXPECT_FALSE(isForbidden(board, Move("Xj12")));
	}
	TEST(TestRenju, ThreeAndThree)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ O X ! X _ _ _ _ _ X _ ! X _\n" /*  2 */
					/*  3 */" _ _ _ X _ _ _ _ _ _ _ _ X _ _\n" /*  3 */
					/*  4 */" _ _ _ X _ _ _ _ _ _ _ _ X _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ X X _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ X _ _ _ _ _ _ _ _ _ _ X _\n" /* 10 */
					/* 11 */" _ _ _ X _ X _ _ _ _ _ _ X _ _\n" /* 11 */
					/* 12 */" _ _ _ _ ! _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ X _ _ _ _ X X ! _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xd2")));
		EXPECT_TRUE(isForbidden(board, Move("Xm2")));
		EXPECT_TRUE(isForbidden(board, Move("Xh6")));
		EXPECT_TRUE(isForbidden(board, Move("Xe12")));
		EXPECT_TRUE(isForbidden(board, Move("Xk13")));
	}
	TEST(TestRenju, DeadThree)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ X X _ ! _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xj5")));
	}
	TEST(TestRenju, ForbiddenPatterns)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /*  1 */
					/*  2 */" _ _ _ _ X _ _ _ _ _ _ _ _ ! _\n" /*  2 */
					/*  3 */" X _ _ ! X X _ _ X _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ X _ _ _ _ _ _ _ _ X _ X _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _ _ X _ _ X _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _ X _ _ _ X _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ O _ _ _ _ O _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ X _ _ _ _ _ _ _ O X O _ _\n" /* 10 */
					/* 11 */" _ _ X ! _ X _ _ _ _ O _ O _ _\n" /* 11 */
					/* 12 */" _ _ _ _ X _ _ _ _ _ X ! X _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ O O X O O _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ O _ _ _ O _ O _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xn2")));
		EXPECT_FALSE(isForbidden(board, Move("Xd3")));
		EXPECT_TRUE(isForbidden(board, Move("Xd11")));
		EXPECT_TRUE(isForbidden(board, Move("Xl12")));
	}
	TEST(TestRenju, TwoDeathEnds)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ X ! X _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ X O X O X _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ O X O X O X O _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ O _ _ _ O _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xj6")));
	}
	TEST(TestRenju, ChainedForbidden)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ O _ _ _ _ _ O _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ X _ _ _ _ _ X _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ ! X _ _ _ _ _ X ! _ _ _\n" /*  5 */
					/*  6 */" _ _ X X ! _ _ _ _ _ ! X X _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ X _ _ _ X _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ X _ X _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ ! X ! X ! _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ X X X ! X X X _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xd5")));
		EXPECT_TRUE(isForbidden(board, Move("Xl5")));
		EXPECT_TRUE(isForbidden(board, Move("Xe6")));
		EXPECT_TRUE(isForbidden(board, Move("Xk6")));
		EXPECT_TRUE(isForbidden(board, Move("Xh9")));
		EXPECT_TRUE(isForbidden(board, Move("Xf12")));
		EXPECT_FALSE(isForbidden(board, Move("Xh12")));
		EXPECT_TRUE(isForbidden(board, Move("Xj12")));
		EXPECT_TRUE(isForbidden(board, Move("Xh13")));

		Board::undoMove(board, Move("Oe3"));
		EXPECT_TRUE(isForbidden(board, Move("Xh12")));
	}
	TEST(TestRenju, LegalMove)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ X _ O X _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ O ! X _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ O X O _ X _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ X O O X O O _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ O X O X X _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ X O _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ O _ X _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xh4")));
	}
	TEST(TestRenju, No33BlockedBy44)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ O O X O O _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ X _ _ X _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ X O _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ X X X _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xg7")));
	}
	TEST(TestRenju, MultipleForks)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ O X O _\n" /*  0 */
					/*  1 */" _ X _ _ _ _ _ _ _ _ X O X O X\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ O X X X O\n" /*  2 */
					/*  3 */" _ _ X ! X _ _ _ _ _ _ _ ! _ _\n" /*  3 */
					/*  4 */" _ _ X O X _ _ _ _ _ _ X _ _ _\n" /*  4 */
					/*  5 */" _ X O _ O _ _ _ _ _ _ _ _ _ X\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  9 */
					/* 10 */" _ X X ! X O _ _ _ _ X _ ! X _\n" /* 10 */
					/* 11 */" _ _ O X X O _ _ _ _ O X X O O\n" /* 11 */
					/* 12 */" _ _ O X O X _ _ _ _ X O _ _ _\n" /* 12 */
					/* 13 */" _ _ _ X _ _ _ _ _ X _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xd3")));
		EXPECT_TRUE(isForbidden(board, Move("Xm3")));
		EXPECT_TRUE(isForbidden(board, Move("Xd10")));
		EXPECT_TRUE(isForbidden(board, Move("Xm10")));
	}
	TEST(TestRenju, WinInFork)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ O X O _\n" /*  0 */
					/*  1 */" _ X _ _ _ _ _ _ _ _ X O X O X\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ O X X X O\n" /*  2 */
					/*  3 */" X X X ! X _ _ _ _ _ _ _ ! _ _\n" /*  3 */
					/*  4 */" _ _ X O X _ _ _ _ _ _ X _ X _\n" /*  4 */
					/*  5 */" _ X O _ O _ _ _ _ _ _ _ _ _ X\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ X _\n" /*  9 */
					/* 10 */" X X X ! X O _ _ _ _ X _ ! X _\n" /* 10 */
					/* 11 */" _ _ O X X O _ _ _ _ O X X O O\n" /* 11 */
					/* 12 */" _ _ O X O X _ _ _ _ X O _ _ _\n" /* 12 */
					/* 13 */" _ _ _ X _ _ _ _ _ X _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Xd3")));
		EXPECT_FALSE(isForbidden(board, Move("Xm3")));
		EXPECT_FALSE(isForbidden(board, Move("Xd10")));
		EXPECT_FALSE(isForbidden(board, Move("Xm10")));
	}
	TEST(TestRenju, ForcedForbidden)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ X _ ! _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ O O X X _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ X O O ! O _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ X O X X _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ ! _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xk7")));

		Board::putMove(board, Move("Xk5"));
		Board::putMove(board, Move("Oi10"));

		EXPECT_FALSE(isForbidden(board, Move("Xk7")));

		Board::putMove(board, Move("Xi9"));
		Board::putMove(board, Move("Om7"));

		EXPECT_TRUE(isForbidden(board, Move("Xk7")));
	}
	TEST(TestRenju, ForcedForbiddenOpponent)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ O _ O _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ X X O O _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ O X X ! X _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ O X O O _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ X O _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_FALSE(isForbidden(board, Move("Ok7")));
	}
	TEST(TestRenju, RemoveForbidden)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ X _ O _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ X _ O X X _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ O _ O O X O X _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ O X _ _ O _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xh5")));

		Board::putMove(board, Move("Xh3"));
		Board::putMove(board, Move("Oi2"));

		EXPECT_FALSE(isForbidden(board, Move("Xh5")));
	}
	TEST(TestRenju, Fake433)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ X O _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ ! _ X _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ X _ X _ O _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" O X _ X X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ X X ! _ _ _ _ _ _ _ _ _ _\n" /*  6 */
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

		EXPECT_FALSE(isForbidden(board, Move("Xc2")));
		EXPECT_FALSE(isForbidden(board, Move("Xe6")));
	}
	TEST(TestRenju, FoulR2225)
	{
		// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ O _ X _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ X O X _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ O X ! _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ ! X X _ O _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ X ! O _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ O _ X _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on

		EXPECT_TRUE(isForbidden(board, Move("Xf8")));

		Board::putMove(board, Move("Xi7"));

		EXPECT_FALSE(isForbidden(board, Move("Xf8")));

		Board::putMove(board, Move("Og9"));

		EXPECT_TRUE(isForbidden(board, Move("Xf8")));
	}
//	TEST(TestRenju, Placeholder)
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

