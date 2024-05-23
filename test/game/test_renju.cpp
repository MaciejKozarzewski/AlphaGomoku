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
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <gtest/gtest.h>

namespace
{
	using namespace ag;

	class TestRenju: public ::testing::Test
	{
		protected:
			PatternCalculator calc;
			matrix<Sign> board;

			TestRenju() :
					calc(GameConfig(GameRules::RENJU, 15, 15))
			{
			}
			void set_board(const std::string &str)
			{
				board = Board::fromString(str);
				calc.setBoard(board, Sign::CROSS);
			}
			void add_move(Move move)
			{
				Board::putMove(board, move);
				calc.addMove(move);
			}
			void undo_move(Move move)
			{
				Board::undoMove(board, move);
				calc.undoMove(move);
			}
			bool is_forbidden(Move move)
			{
				const bool tmp1 = isForbidden(board, move);
				const bool tmp2 = calc.isForbidden(move.sign, move.row, move.col);
				EXPECT_EQ(tmp1, tmp2);
				return tmp1;
			}
	};
}

namespace ag
{
	TEST_F(TestRenju, WinIn1)
	{
// @formatter:off
		set_board(
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

		EXPECT_FALSE(is_forbidden(Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_FALSE(is_forbidden(Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CROSS_WIN);
	}
	TEST_F(TestRenju, Overline)
	{
// @formatter:off
		set_board(
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

		EXPECT_FALSE(is_forbidden(Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_TRUE(is_forbidden(Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CIRCLE_WIN);
	}
	TEST_F(TestRenju, WinInsideOverline)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xa0")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa0")), GameOutcome::CROSS_WIN);

		EXPECT_FALSE(is_forbidden(Move("Xa5")));
		EXPECT_EQ(getOutcome(GameRules::RENJU, board, Move("Xa5")), GameOutcome::CROSS_WIN);
	}
	TEST_F(TestRenju, FourAndFour)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xk0")));
		EXPECT_TRUE(is_forbidden(Move("Xd1")));
		EXPECT_TRUE(is_forbidden(Move("Xk4")));
		EXPECT_TRUE(is_forbidden(Move("Xa9")));
		EXPECT_TRUE(is_forbidden(Move("Xi9")));
		EXPECT_TRUE(is_forbidden(Move("Xj14")));
	}
	TEST_F(TestRenju, FourAndFreeThree)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xd1")));
		EXPECT_FALSE(is_forbidden(Move("Xl1")));
		EXPECT_FALSE(is_forbidden(Move("Xh6")));
		EXPECT_FALSE(is_forbidden(Move("Xc12")));
		EXPECT_FALSE(is_forbidden(Move("Xj12")));
	}
	TEST_F(TestRenju, ThreeAndThree)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xd2")));
		EXPECT_TRUE(is_forbidden(Move("Xm2")));
		EXPECT_TRUE(is_forbidden(Move("Xh6")));
		EXPECT_TRUE(is_forbidden(Move("Xe12")));
		EXPECT_TRUE(is_forbidden(Move("Xk13")));
	}
	TEST_F(TestRenju, DeadThree)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xj5")));
	}
	TEST_F(TestRenju, ForbiddenPatterns)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xn2")));
		EXPECT_FALSE(is_forbidden(Move("Xd3")));
		EXPECT_TRUE(is_forbidden(Move("Xd11")));
		EXPECT_TRUE(is_forbidden(Move("Xl12")));
	}
	TEST_F(TestRenju, TwoDeathEnds)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xj6")));
	}
	TEST_F(TestRenju, ChainedForbidden)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xd5")));
		EXPECT_TRUE(is_forbidden(Move("Xl5")));
		EXPECT_TRUE(is_forbidden(Move("Xe6")));
		EXPECT_TRUE(is_forbidden(Move("Xk6")));
		EXPECT_TRUE(is_forbidden(Move("Xh9")));
		EXPECT_TRUE(is_forbidden(Move("Xf12")));
		EXPECT_FALSE(is_forbidden(Move("Xh12")));
		EXPECT_TRUE(is_forbidden(Move("Xj12")));
		EXPECT_TRUE(is_forbidden(Move("Xh13")));

		undo_move(Move("Oe3"));
		EXPECT_TRUE(is_forbidden(Move("Xh12")));
	}
	TEST_F(TestRenju, LegalMove)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xh4")));
	}
	TEST_F(TestRenju, No33BlockedBy44)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xg7")));
	}
	TEST_F(TestRenju, MultipleForks)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xd3")));
		EXPECT_TRUE(is_forbidden(Move("Xm3")));
		EXPECT_TRUE(is_forbidden(Move("Xd10")));
		EXPECT_TRUE(is_forbidden(Move("Xm10")));
	}
	TEST_F(TestRenju, WinInFork)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xd3")));
		EXPECT_FALSE(is_forbidden(Move("Xm3")));
		EXPECT_FALSE(is_forbidden(Move("Xd10")));
		EXPECT_FALSE(is_forbidden(Move("Xm10")));
	}
	TEST_F(TestRenju, ForcedForbidden)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xk7")));

		add_move(Move("Xk5"));
		add_move(Move("Oi10"));

		EXPECT_FALSE(is_forbidden(Move("Xk7")));

		add_move(Move("Xi9"));
		add_move(Move("Om7"));

		EXPECT_TRUE(is_forbidden(Move("Xk7")));
	}
	TEST_F(TestRenju, ForcedForbiddenOpponent)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Ok7")));
	}
	TEST_F(TestRenju, RemoveForbidden)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xh5")));

		add_move(Move("Xh3"));
		add_move(Move("Oi2"));

		EXPECT_FALSE(is_forbidden(Move("Xh5")));
	}
	TEST_F(TestRenju, Fake433)
	{
// @formatter:off
		set_board(
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


		EXPECT_FALSE(is_forbidden(Move("Xc2")));
		EXPECT_FALSE(is_forbidden(Move("Xe6")));
	}
	TEST_F(TestRenju, FoulR2225)
	{
// @formatter:off
		set_board(
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


		EXPECT_TRUE(is_forbidden(Move("Xf8")));

		add_move(Move("Xi7"));

		EXPECT_FALSE(is_forbidden(Move("Xf8")));

		add_move(Move("Og9"));

		EXPECT_TRUE(is_forbidden(Move("Xf8")));
	}

//	TEST_F(TestRenju, Placeholder)
//	{
//// @formatter:off
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

