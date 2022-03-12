/*
 * test_renju.cpp
 *
 *  Created on: Aug 19, 2021
 *  	Author: Mira Fontan
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>

#include <gtest/gtest.h>

namespace ag
{
//	TEST(TestRenju, WinIn1)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xa0")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (0, 0), Square::eCombThreat::eWinIn1);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xa5")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (0, 5), Square::eCombThreat::eWinIn1);
//	}
//	TEST(TestRenju, Overline)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xa0")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 0, 0 ), Square::eCombThreat::eWinIn1 );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xa5")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 0, 5 ), Square::eCombThreat::eForbidden );
//	}
//	TEST(TestRenju, WinInsideOverline)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//	    Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" ! _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" ! X X X X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xa0")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 0, 0 ), Square::eCombThreat::eWinIn1 );
//
//		EXPECT_FALSE(board.isForbidden(Move("Xa5")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (0, 5), Square::eCombThreat::eWinIn1);
//	}
//	TEST(TestRenju, FourAndFour)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ X ! _ X X _\n" /*  0 */
//					/*  1 */" _ _ X ! X X O _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  2 */
//					/*  3 */" _ _ _ X _ _ _ _ _ _ _ _ _ X _\n" /*  3 */
//					/*  4 */" _ _ _ X _ _ O X X X ! _ _ _ X\n" /*  4 */
//					/*  5 */" _ _ _ X _ _ _ _ _ _ X _ _ _ _\n" /*  5 */
//					/*  6 */" X _ _ O _ _ _ _ _ _ X _ _ _ _\n" /*  6 */
//					/*  7 */" X _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /*  8 */
//					/*  9 */" ! _ _ _ _ X _ X ! X _ X _ _ _\n" /*  9 */
//					/* 10 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" O _ _ _ _ X X X _ ! _ X X X _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk0")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 0, 9 ), Square::eCombThreat::eForbidden );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xd1")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 4, 1 ), Square::eCombThreat::eForbidden );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk4")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 8,12 ), Square::eCombThreat::eForbidden );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xa9")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>(10, 4 ), Square::eCombThreat::eForbidden );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xi9")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>(10, 7 ), Square::eCombThreat::eForbidden );
//
//		EXPECT_TRUE(board.isForbidden(Move("Xj14")));
//	}
//	TEST(TestRenju, FourAndFreeThree)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ ! _ _ _ _ O _ X ! X _ O\n" /*  1 */
//					/*  2 */" _ _ X X _ _ _ _ _ _ _ X _ _ _\n" /*  2 */
//					/*  3 */" _ X _ X _ _ _ _ _ _ _ X _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ X _ _ _ _ _ _ _ X _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ O _ _ _ _ _ _ _ O _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ X X ! _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" X _ _ _ _ _ _ O _ _ _ X _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ ! _ X X _ _ _ ! _ X X X O\n" /* 12 */
//					/* 13 */" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xd1")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (2, 12), Square::eCombThreat::eFourFreeThree);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xl1")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (3, 1), Square::eCombThreat::eFourFreeThree);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xh6")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (7, 6), Square::eCombThreat::eFourFreeThree);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xc12")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (9, 12), Square::eCombThreat::eFourFreeThree);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xj12")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (11, 1), Square::eCombThreat::eFourFreeThree);
//	}
//	TEST(TestRenju, ThreeAndThree)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ O X ! X _ _ _ _ _ X _ ! X _\n" /*  2 */
//					/*  3 */" _ _ _ X _ _ _ _ _ _ _ _ X _ _\n" /*  3 */
//					/*  4 */" _ _ _ X _ _ _ _ _ _ _ _ X _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ X X _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ X _ _ _ _ _ _ _ _ _ _ X _\n" /* 10 */
//					/* 11 */" _ _ _ X _ X _ _ _ _ _ _ X _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ ! _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ X _ _ _ _ X X ! _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xd2")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (3, 2), Square::eCombThreat::eTwoThrees);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xm2")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (4, 12), Square::eCombThreat::eTwoThrees);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xh6")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (7, 6), Square::eCombThreat::eTwoThrees);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xe12")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (10, 13), Square::eCombThreat::eTwoThrees);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk13")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (12, 2), Square::eCombThreat::eTwoThrees);
//	}
//	TEST(TestRenju, DeadThree)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ X X _ ! _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xj5")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (9, 5), Square::eCombThreat::eTwoThrees);
//	}
//	TEST(TestRenju, ForbiddenPatterns)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X\n" /*  1 */
//					/*  2 */" _ _ _ _ X _ _ _ _ _ _ _ _ ! _\n" /*  2 */
//					/*  3 */" x _ _ ! X X _ _ X _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ X _ _ _ _ _ _ _ _ X _ X _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _ _ X _ _ X _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ X _ _ _ X _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ O _ _ _ _ O _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ X _ _ _ _ _ _ _ O X O _ _\n" /* 10 */
//					/* 11 */" _ _ X ! _ X _ _ _ _ O _ O _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ X _ _ _ _ _ X ! X _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ O O X O O _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ O _ _ _ O _ O _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xn2")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (13, 2), Square::eCombThreat::eSingleFour);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xd3")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (3, 3), Square::eCombThreat::eThreeDistThree);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xd11")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (3, 11), Square::eCombThreat::eTwoThrees);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xl12")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (11, 12), Square::eCombThreat::eTwoThrees);
//	}
//	TEST(TestRenju, TwoDeathEnds)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ X ! X _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ X O X O X _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ O X O X O X O _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ O _ _ _ O _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xj6")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (9, 6), Square::eCombThreat::eForbidden);
//	}
//	TEST(TestRenju, ChainedForbidden)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ O _ _ _ _ _ O _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ X _ _ _ _ _ X _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ ! X _ _ _ _ _ X ! _ _ _\n" /*  5 */
//					/*  6 */" _ _ X X ! _ _ _ _ _ ! X X _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ X _ _ _ X _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ X _ X _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ ! _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ ! X ! X ! _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ X X X ! X X X _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xd5")));
//		EXPECT_TRUE(board.isForbidden(Move("Xk5")));
//		EXPECT_TRUE(board.isForbidden(Move("Xe6")));
//		EXPECT_TRUE(board.isForbidden(Move("Xk6")));
//		EXPECT_TRUE(board.isForbidden(Move("Xh9")));
//		EXPECT_TRUE(board.isForbidden(Move("Xf12")));
//		EXPECT_FALSE(board.isForbidden(Move("Xh12")));
//		EXPECT_TRUE(board.isForbidden(Move("Xj12")));
//		EXPECT_TRUE(board.isForbidden(Move("Xh13")));
//
//		board.at(3, 4) = Sign::NONE; // undo move "Oe3"
//		EXPECT_TRUE(board.isForbidden(Move("Xh12")));
//	}
//	TEST(TestRenju, LegalMove)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ X _ O X _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ O ! X _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ O X O _ X _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ X O O X O O _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ O X O X X _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ X O _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ O _ X _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xh4")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (7, 4), Square::eCombThreat::eThreeDistThree);
//	}
//	TEST(TestRenju, No33BlockedBy44)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ O O X O O _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ X _ _ X _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ X O _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ X X X _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xg7")));
//	}
//	TEST(TestRenju, MultipleForks)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ O X O _\n" /*  0 */
//					/*  1 */" _ X _ _ _ _ _ _ _ _ X O X O X\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ O X X X O\n" /*  2 */
//					/*  3 */" _ _ X ! X _ _ _ _ _ _ _ ! _ _\n" /*  3 */
//					/*  4 */" _ _ X O X _ _ _ _ _ _ X _ _ _\n" /*  4 */
//					/*  5 */" _ X O _ O _ _ _ _ _ _ _ _ _ X\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" /*  9 */
//					/* 10 */" _ X X ! X O _ _ _ _ X _ ! X _\n" /* 10 */
//					/* 11 */" _ _ O X X O _ _ _ _ O X X O O\n" /* 11 */
//					/* 12 */" _ _ O X O X _ _ _ _ X O _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ X _ _ _ _ _ X _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xd3")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (3, 3), Square::eCombThreat::eForbidden);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xm3")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (12, 3), Square::eCombThreat::eForbidden);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xd10")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (3, 10), Square::eCombThreat::eForbidden);
//
//		EXPECT_TRUE(board.isForbidden(Move("Xm19")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (12, 10), Square::eCombThreat::eForbidden);
//	}
//	TEST(TestRenju, WinInFork)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ O X O _\n" /*  0 */
//					/*  1 */" _ X _ _ _ _ _ _ _ _ X O X O X\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ O X X X O\n" /*  2 */
//					/*  3 */" X X X ! X _ _ _ _ _ _ _ ! _ _\n" /*  3 */
//					/*  4 */" _ _ X O X _ _ _ _ _ _ X _ X _\n" /*  4 */
//					/*  5 */" _ X O _ O _ _ _ _ _ _ _ _ _ X\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ X _\n" /*  9 */
//					/* 10 */" X X X ! X O _ _ _ _ X _ ! X _\n" /* 10 */
//					/* 11 */" _ _ O X X O _ _ _ _ O X X O O\n" /* 11 */
//					/* 12 */" _ _ O X O X _ _ _ _ X O _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ X _ _ _ _ _ X _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xd3")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 3, 3 ), Square::eCombThreat::eWinIn1 );
//
//		EXPECT_FALSE(board.isForbidden(Move("Xm3")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>( 3,10 ), Square::eCombThreat::eWinIn1 );
//
//		EXPECT_FALSE(board.isForbidden(Move("Xd10")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>(12, 3 ), Square::eCombThreat::eWinIn1 );
//
//		EXPECT_FALSE(board.isForbidden(Move("Xm10")));
////	    CHECK_INTEGERS_EQ( b.GetSquareStatus<eMove_t::eXX>(12,10 ), Square::eCombThreat::eWinIn1 );
//	}
//	TEST(TestRenju, ForcedForbidden)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ X _ ! _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ O O X X _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ X O O ! O _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ X O X X _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ ! _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk7")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (10, 7), Square::eCombThreat::eTwoThrees);
//
//		board.putMove(Move("Xk5"));
//		board.putMove(Move("Oi10"));
//
//		EXPECT_FALSE(board.isForbidden(Move("Xk7")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (10, 7), Square::eCombThreat::eTwoFours); FIXME shouldn't it be 4x3 fork?
//
//		board.putMove(Move("Xi9"));
//		board.putMove(Move("Om7"));
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk7")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (10, 7), Square::eCombThreat::eFourFreeThree);
//	}
//	TEST(TestRenju, ForcedForbiddenOpponent)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ O _ O _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ X X O O _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ O X X ! X _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ O X O O _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ X O _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xk7")));
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eOO > (10, 7), Square::eCombThreat::eTwoThrees);
//	}
//	TEST(TestRenju, RemoveForbidden)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ X _ O _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ X _ O X X _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ O _ O O X O X _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ O X _ _ O _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xh5")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (7, 5), Square::eCombThreat::eForbidden);
//
//		board.putMove(Move("Xh3"));
//		board.putMove(Move("Oi2"));
//
//		EXPECT_FALSE(board.isForbidden(Move("Xh5")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (7, 5), Square::eCombThreat::eFourFreeThree);
//	}
//	TEST(TestRenju, Fake433)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ X O _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ ! _ X _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ X _ X _ O _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" O X _ X X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ X X ! _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_FALSE(board.isForbidden(Move("Xc2")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (2, 2), Square::eCombThreat::eThreeDistThree);
//
//		EXPECT_FALSE(board.isForbidden(Move("Xe6")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (4, 6), Square::eCombThreat::eFourFreeThree);
//	}
//	TEST(TestRenju, Fake43)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ O _ _ O _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ O O X _ X _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ O X O O O _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ X X O O _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ X X X O _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ X _ _ O _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ X X _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (10, 11), Square::eCombThreat::eFourDistThree);
////		CHECK_INTEGERS_NE(b.GetSquareStatus < eMove_t::eXX > (10, 11), Square::eCombThreat::eFourFreeThree);
//	}
//	TEST(TestRenju, FoulR2225)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ O _ X _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ X O X _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ O X ! _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ ! X X _ O _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ X ! O _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ O _ X _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(board.isForbidden(Move("Xf8")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (5, 8), Square::eCombThreat::eForbidden);
//
//		board.putMove(Move("Xi7"));
//
//		EXPECT_FALSE(board.isForbidden(Move("Xf8")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (5, 8), Square::eCombThreat::eThreeDistThree);
//
//		board.putMove(Move("Og9"));
//
//		EXPECT_TRUE(board.isForbidden(Move("Xf8")));
////		CHECK_INTEGERS_EQ(b.GetSquareStatus < eMove_t::eXX > (5, 8), Square::eCombThreat::eForbidden);
//	}
//	TEST(TestRenju, WhiteOverline)
//	{
//		GTEST_SKIP();
//		// @formatter:off
//		Board board(/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */, Sign::CIRCLE, GameRules::RENJU); // @formatter:on
//
////		CHECK_INTEGERS_EQ(bX.GetSquareStatus < eMove_t::eOO > (0, 4), Square::eCombThreat::eSingleFour);
////		CHECK_INTEGERS_EQ(bX.GetSquareStatus < eMove_t::eOO > (0, 5), Square::eCombThreat::eSingleFour);
//
//		board.putMove(Move("Oa5"));
//
////		CHECK_INTEGERS_EQ(bX.GetSquareStatus < eMove_t::eOO > (0, 4), Square::eCombThreat::eWinIn1);
//	}
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
//					/*        a b c d e f g h i j k l m n o          */, Sign::CROSS, GameRules::RENJU); // @formatter:on
//
//		EXPECT_TRUE(true);
//	}
} /* namespace ag */

