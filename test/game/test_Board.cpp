/*
 * test_Board.cpp
 *
 *  Created on: Aug 19, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/rules.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestBoard, is_full)
	{
		matrix<Sign> board(9, 11);
		EXPECT_FALSE(Board::isFull(board));
		board.fill(Sign::CROSS);
		EXPECT_TRUE(Board::isFull(board));
		board.at(4, 5) = Sign::NONE;
		EXPECT_FALSE(Board::isFull(board));
	}
	TEST(TestBoard, is_empty)
	{
		matrix<Sign> board(9, 11);
		EXPECT_TRUE(Board::isEmpty(board));
		board.at(4, 5) = Sign::CROSS;
		EXPECT_FALSE(Board::isEmpty(board));
		board.fill(Sign::CROSS);
		EXPECT_FALSE(Board::isEmpty(board));
	}
	TEST(TestBoard, empty_from_string)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
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

		Sign sign_to_move = Sign::CROSS;

		EXPECT_TRUE(Board::isEmpty(board));
		EXPECT_TRUE(Board::isValid(board, sign_to_move));
		EXPECT_FALSE(Board::isFull(board));
		EXPECT_EQ(board.rows(), 15);
		EXPECT_EQ(board.cols(), 15);
	}
	TEST(TestBoard, from_string)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o p q r s t          */
					/*  0 */" _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ X _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/* 15 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 15 */
					/* 16 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 16 */
					/* 17 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 17 */
					/* 18 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 18 */
					/* 19 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 19 */
					/*        a b c d e f g h i j k l m n o p q r s t          */);
// @formatter:on

		Sign sign_to_move = Sign::CIRCLE;

		EXPECT_FALSE(Board::isEmpty(board));
		EXPECT_TRUE(Board::isValid(board, sign_to_move));
		EXPECT_FALSE(Board::isFull(board));
		EXPECT_EQ(board.rows(), 20);
		EXPECT_EQ(board.cols(), 20);

		EXPECT_EQ(Board::getSignAt(board, Move("Ob0")), Sign::CIRCLE);
		EXPECT_EQ(Board::getSignAt(board, Move("Xb2")), Sign::CROSS);
		EXPECT_EQ(Board::getSignAt(board, Move("Ob3")), Sign::CIRCLE);
		EXPECT_EQ(Board::getSignAt(board, Move("Xb4")), Sign::CROSS);
		EXPECT_EQ(Board::getSignAt(board, Move("Xd4")), Sign::CROSS);
	}
	TEST(TestBoard, invalid_from_string)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ X _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ X _ X _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
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

		Sign sign_to_move = Sign::CIRCLE;

		EXPECT_FALSE(Board::isValid(board, sign_to_move));
	}
	TEST(TestBoard, is_possible)
	{
// @formatter:off
		matrix<Sign> current = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
				    /*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

// @formatter:off
		matrix<Sign> next1 = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ X _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

// @formatter:off
		matrix<Sign> next2 = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
					/*  1 */" _ _ X _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

// @formatter:off
		matrix<Sign> next3 = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
				    /*  1 */" _ _ X _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		EXPECT_FALSE(Board::isTransitionPossible(current, next1));
		EXPECT_FALSE(Board::isTransitionPossible(current, next2));
		EXPECT_TRUE(Board::isTransitionPossible(current, next3));
	}

} /* namespace ag */

