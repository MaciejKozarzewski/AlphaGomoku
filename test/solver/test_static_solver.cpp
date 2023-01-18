/*
 * test_static_solver.cpp
 *
 *  Created on: Apr 21, 2022
 *      Author: Maciej Kozarzewski
 */

//#include <alphagomoku/game/Board.hpp>
//#include <alphagomoku/game/rules.hpp>
//#include <alphagomoku/solver/VCTSolver.hpp>
//#include <alphagomoku/mcts/SearchTask.hpp>
//#include <alphagomoku/mcts/Edge.hpp>
//
//#include <gtest/gtest.h>
//
//#include <algorithm>
//
//namespace
//{
//	using namespace ag;
//	bool contains(const SearchTask &task, Move m)
//	{
//		return std::any_of(task.getProvenEdges().begin(), task.getProvenEdges().end(), [m](const Edge &edge)
//		{	return edge.getMove() == m;});
//	}
//	bool contains(const SearchTask &task, Move m, ProvenValue pv)
//	{
//		return std::any_of(task.getProvenEdges().begin(), task.getProvenEdges().end(), [m, pv](const Edge &edge)
//		{	return edge.getMove() == m and edge.getProvenValue() == pv;});
//	}
//}
//
//namespace ag
//{
//	using ag::experimental::VCTSolver;
//
//	TEST(TestStaticSolver, empty_board)
//	{
//		GTEST_SKIP();
//		GameConfig cfg(GameRules::FREESTYLE, 8, 8);
//		SearchTask task(cfg.rules);
//		VCTSolver solver(cfg);
//
//		// @formatter:off
//		task.set(Board::fromString(	/*        a b c d e f g h          */
//									/*  0 */" _ _ _ _ _ _ _ _\n" /*  0 */
//									/*  1 */" _ _ _ _ _ _ _ _\n" /*  1 */
//									/*  2 */" _ _ _ _ _ _ _ _\n" /*  2 */
//									/*  3 */" _ _ _ _ _ _ _ _\n" /*  3 */
//									/*  4 */" _ _ _ _ _ _ _ _\n" /*  4 */
//									/*  5 */" _ _ _ _ _ _ _ _\n" /*  5 */
//									/*  6 */" _ _ _ _ _ _ _ _\n" /*  6 */
//									/*  7 */" _ _ _ _ _ _ _ _\n" /*  7 */
//									/*        a b c d e f g h          */), Sign::CROSS);          // @formatter:on
//
//		solver.solve(task, 0);
//		EXPECT_FALSE(task.isReady());
//	}
//
//	TEST(TestStaticSolver, win_in_1)
//	{
//		GTEST_SKIP();
//		GameConfig cfg(GameRules::FREESTYLE, 10, 10);
//		SearchTask task(cfg.rules);
//		VCTSolver solver(cfg);
//
//		// @formatter:off
//		task.set(Board::fromString(	/*        a b c d e f g h i j          */
//									/*  0 */" X _ _ _ _ X _ _ _ _\n" /*  0 */
//									/*  1 */" X _ _ _ _ _ X _ _ _\n" /*  1 */
//									/*  2 */" X _ _ _ _ _ _ _ _ _\n" /*  2 */
//									/*  3 */" X _ _ _ _ _ _ _ X _\n" /*  3 */
//									/*  4 */" _ _ _ _ X _ _ _ _ X\n" /*  4 */
//									/*  5 */" _ _ _ X _ _ _ _ _ _\n" /*  5 */
//									/*  6 */" _ _ X _ _ _ _ _ _ _\n" /*  6 */
//									/*  7 */" _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//									/*  8 */" X _ _ _ O X X X X _\n" /*  9 */
//									/*  9 */" _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//									/*        a b c d e f g h i j          */), Sign::CROSS);          // @formatter:on
//
//		solver.solve(task, 0);
//		EXPECT_TRUE(task.isReady());
//		EXPECT_EQ(task.getEdges().size(), 4u);
//		EXPECT_TRUE(contains(task, Move("Xa4"), ProvenValue::WIN));
//		EXPECT_TRUE(contains(task, Move("Xb7"), ProvenValue::WIN));
//		EXPECT_TRUE(contains(task, Move("Xh2"), ProvenValue::WIN));
//		EXPECT_TRUE(contains(task, Move("Xe8"), ProvenValue::WIN));
//	}
//
//	TEST(TestStaticSolver, placeholder)
//	{
//		GTEST_SKIP();
//		GameConfig cfg(GameRules::FREESTYLE, 8, 8);
//		SearchTask task(cfg.rules);
//		VCTSolver solver(cfg);
//
//		// @formatter:off
//		task.set(Board::fromString(
//					/*        a b c d e f g h i j k l m n o          */
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
//					/*        a b c d e f g h i j k l m n o          */), Sign::CROSS);                 // @formatter:on
//
////		EXPECT_TRUE(Board::isEmpty(board));
////		EXPECT_TRUE(Board::isValid(board, sign_to_move));
////		EXPECT_FALSE(Board::isFull(board));
////		EXPECT_EQ(board.rows(), 15);
////		EXPECT_EQ(board.cols(), 15);
//	}
//
//} /* namespace ag */

