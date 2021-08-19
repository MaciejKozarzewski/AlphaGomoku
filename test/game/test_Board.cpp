/*
 * test_Board.cpp
 *
 *  Created on: Aug 19, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestBoard, empty_from_string)
	{
		// @formatter:off
		Board board(/*        a b c d e f g h i j k l m n o          */
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
					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::STANDARD); // @formatter:on

		EXPECT_TRUE(board.isEmpty());
		EXPECT_FALSE(board.isFull());
		EXPECT_EQ(board.rows(), 15);
		EXPECT_EQ(board.cols(), 15);
		EXPECT_EQ(board.signToMove(), Sign::CROSS);
		EXPECT_EQ(board.rules(), GameRules::STANDARD);
	}
	TEST(TestBoard, from_string)
	{
		// @formatter:off
		Board board(/*        a b c d e f g h i j k l m n o p q r s t          */
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
					/*        a b c d e f g h i j k l m n o p q r s t          */, Sign::CIRCLE, GameRules::FREESTYLE); // @formatter:on

		EXPECT_FALSE(board.isEmpty());
		EXPECT_FALSE(board.isFull());
		EXPECT_EQ(board.rows(), 20);
		EXPECT_EQ(board.cols(), 20);
		EXPECT_EQ(board.signToMove(), Sign::CIRCLE);
		EXPECT_EQ(board.rules(), GameRules::FREESTYLE);

		EXPECT_EQ(board.at("b0"), Sign::CIRCLE);
		EXPECT_EQ(board.at("b2"), Sign::CROSS);
		EXPECT_EQ(board.at("b3"), Sign::CIRCLE);
		EXPECT_EQ(board.at("b4"), Sign::CROSS);
		EXPECT_EQ(board.at("d4"), Sign::CROSS);
	}
} /* namespace ag */

