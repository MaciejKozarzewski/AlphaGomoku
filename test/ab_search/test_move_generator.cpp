/*
 * test_move_generator.cpp
 *
 *  Created on: Oct 2, 2022
 *		Author: Mira Fontan
 *      Author: Maciej Kozarzewski
 */

//#include <alphagomoku/game/Board.hpp>
//#include <alphagomoku/game/Move.hpp>
//#include <alphagomoku/game/rules.hpp>
//#include <alphagomoku/ab_search/MoveGenerator.hpp>
//
//#include <gtest/gtest.h>
//
//namespace
//{
//	class MoveGenWrapper
//	{
//			ag::GameConfig game_config;
//			ag::PatternCalculator pattern_calculator;
//			ag::MoveGenerator move_generator;
//		public:
//			MoveGenWrapper(ag::GameRules rules, const ag::matrix<ag::Sign> &board, ag::Sign signToMove) :
//					game_config(rules, board.rows(), board.cols()),
//					pattern_calculator(game_config),
//					move_generator(game_config, pattern_calculator)
//			{
//				pattern_calculator.setBoard(board, signToMove);
//			}
//			ag::MoveGenerator* operator->() noexcept
//			{
//				return &move_generator;
//			}
//	};
//}
//
//namespace ag
//{
//	TEST(TestMoveGenerator, AllMoves)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ X _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::LEGAL);
//		EXPECT_EQ(movegen->count(), 24);
//
//		movegen->generate(MoveGeneratorMode::REDUCED);
//		EXPECT_EQ(movegen->count(), 24);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 24);
//	}
//	TEST(TestMoveGenerator, AllMoves2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" _ ! ! ! !\n" /*  0 */
//					/*  1 */" ! ! ! X !\n" /*  1 */
//					/*  2 */" _ ! ! ! !\n" /*  2 */
//					/*  3 */" _ ! ! ! !\n" /*  3 */
//					/*  4 */" ! _ _ ! _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::LEGAL);
//		EXPECT_EQ(movegen->count(), 24);
//
//		movegen->generate(MoveGeneratorMode::REDUCED);
//		EXPECT_EQ(movegen->count(), 18);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 18);
//	}
//	TEST(TestMoveGenerator, AllMoves3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ O _ X _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ X X _ _ _ _\n" /*  4 */
//					/*  5 */" _ ! ! O O O ! ! _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::LEGAL);
//		EXPECT_EQ(movegen->count(), 74);
//
//		movegen->generate(MoveGeneratorMode::REDUCED);
//		EXPECT_EQ(movegen->count(), 68);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 2);
//		EXPECT_TRUE(movegen->contains(Move("Xc5")));
//		EXPECT_TRUE(movegen->contains(Move("Xg5")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen_caro->count(), 4);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xb5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xc5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xg5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xh5")));
//	}
//	TEST(TestMoveGenerator, SimpleForced)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" X _ _ _ _\n" /*  0 */
//					/*  1 */" X _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _\n" /*  2 */
//					/*  3 */" X _ _ _ _\n" /*  3 */
//					/*  4 */" ! _ _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xa4")));
//	}
//	TEST(TestMoveGenerator, SimpleForced2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" O _ _ _ _\n" /*  0 */
//					/*  1 */" O _ _ _ _\n" /*  1 */
//					/*  2 */" O _ _ _ _\n" /*  2 */
//					/*  3 */" O _ _ _ _\n" /*  3 */
//					/*  4 */" ! _ _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xa4")));
//	}
//	TEST(TestMoveGenerator, SimpleForced3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" O _ X _ _\n" /*  0 */
//					/*  1 */" O _ X _ _\n" /*  1 */
//					/*  2 */" O _ X _ _\n" /*  2 */
//					/*  3 */" O _ X _ _\n" /*  3 */
//					/*  4 */" _ _ ! _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xc4")));
//	}
//	TEST(TestMoveGenerator, Open3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" ! O O O ! ! _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xa0")));
//		EXPECT_TRUE(movegen->contains(Move("Xe0")));
//		EXPECT_TRUE(movegen->contains(Move("Xf0")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//		EXPECT_EQ(movegen_caro->count(), 4);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xa0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xf0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xg0")));
//	}
//	TEST(TestMoveGenerator, InlineFork4x4)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" O O O ! ! ! O O O\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xd0")));
//		EXPECT_TRUE(movegen->contains(Move("Xe0")));
//		EXPECT_TRUE(movegen->contains(Move("Xf0")));
//	}
//	TEST(TestMoveGenerator, InlineFork4x4_v2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" O O O ! ! O O ! O\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xd0")));
//		EXPECT_TRUE(movegen->contains(Move("Xe0")));
//		EXPECT_TRUE(movegen->contains(Move("Xh0")));
//
//		MoveGenWrapper movegen_standard(GameRules::STANDARD, board, Sign::CROSS);
//		movegen_standard->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_standard->isInDanger());
//		EXPECT_FALSE(movegen_standard->isThreatening());
//	}
//	TEST(TestMoveGenerator, InterlacedMove)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ ! O O O ! _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" X X X ! ! _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 4);
//		EXPECT_TRUE(movegen->contains(Move("Xc0")));
//		EXPECT_TRUE(movegen->contains(Move("Xg0")));
//		EXPECT_TRUE(movegen->contains(Move("Xd6")));
//		EXPECT_TRUE(movegen->contains(Move("Xe6")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//		EXPECT_EQ(movegen_caro->count(), 6);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xb0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xc0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xg0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xh0")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd6")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe6")));
//	}
//	TEST(TestMoveGenerator, InterlacedMoveDuplicate)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ ! _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ O _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ O _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ O _ _ _ _\n" /*  5 */
//					/*  6 */" X X X ! ! _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xe2")));
//		EXPECT_TRUE(movegen->contains(Move("Xd6")));
//		EXPECT_TRUE(movegen->contains(Move("Xe6")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//		EXPECT_EQ(movegen_caro->count(), 5);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe1")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe2")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe7")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd6")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe6")));
//	}
//	TEST(TestMoveGenerator, WinIn3Moves_1)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" ! _ X X X\n" /*  0 */
//					/*  1 */" _ _ _ _ _\n" /*  1 */
//					/*  2 */" X _ _ _ _\n" /*  2 */
//					/*  3 */" X _ _ _ _\n" /*  3 */
//					/*  4 */" X _ _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xa0")));
//
//		MoveGenWrapper movegen_caro(GameRules::RENJU, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//		EXPECT_GT(movegen_caro->count(), 1);
//	}
//	TEST(TestMoveGenerator, WinIn3Moves_2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" _ _ O _ _\n" /*  0 */
//					/*  1 */" _ _ O _ _\n" /*  1 */
//					/*  2 */" _ _ O _ _\n" /*  2 */
//					/*  3 */" _ _ ! _ _\n" /*  3 */
//					/*  4 */" ! O ! O O\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xa4")));
//		EXPECT_TRUE(movegen->contains(Move("Xc3")));
//		EXPECT_TRUE(movegen->contains(Move("Xc4")));
//	}
//	TEST(TestMoveGenerator, WinIn3Moves_3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" _ ! _ _ _\n" /*  0 */
//					/*  1 */" ! ! O O O\n" /*  1 */
//					/*  2 */" _ O _ _ _\n" /*  2 */
//					/*  3 */" _ O _ _ _\n" /*  3 */
//					/*  4 */" _ O _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xb0")));
//		EXPECT_TRUE(movegen->contains(Move("Xa1")));
//		EXPECT_TRUE(movegen->contains(Move("Xb1")));
//	}
//	TEST(TestMoveGenerator, WinIn3Moves_4) // TODO this can be improved (it seems only single move is required - Xb1)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f        */
//					/*  0 */" _ ! _ _ _ _\n" /*  0 */
//					/*  1 */" ! ! O O O !\n" /*  1 */
//					/*  2 */" _ O _ _ _ _\n" /*  2 */
//					/*  3 */" _ O _ _ _ _\n" /*  3 */
//					/*  4 */" _ O _ _ _ _\n" /*  4 */
//					/*  5 */" _ ! _ _ _ _\n" /*  5 */
//					/*        a b c d e f        */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 5);
//		EXPECT_TRUE(movegen->contains(Move("Xb0")));
//		EXPECT_TRUE(movegen->contains(Move("Xa1")));
//		EXPECT_TRUE(movegen->contains(Move("Xb1")));
//		EXPECT_TRUE(movegen->contains(Move("Xf1")));
//		EXPECT_TRUE(movegen->contains(Move("Xb5")));
//	}
//	TEST(TestMoveGenerator, WinIn5Moves_3x3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ X _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ ! X X _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ X _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xd4")));
//
//		MoveGenWrapper movegen_renju(GameRules::RENJU, board, Sign::CROSS);
//		movegen_renju->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_renju->isInDanger());
//		EXPECT_FALSE(movegen_renju->isThreatening());
//		EXPECT_FALSE(movegen_renju->contains(Move("Xd4")));
//	}
//	TEST(TestMoveGenerator, WinIn5Moves_4x3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ O _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ X _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ X _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ ! X X _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ X _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ O _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xd4")));
//
//		MoveGenWrapper movegen_renju(GameRules::CARO, board, Sign::CROSS);
//		movegen_renju->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_renju->isInDanger());
//		EXPECT_FALSE(movegen_renju->isThreatening());
//	}
//	TEST(TestMoveGenerator, DefendLossIn6_4x3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ ! _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ O _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ ! ! O O O\n" /*  4 */
//					/*  5 */" _ _ _ _ O _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ ! _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 4);
//		EXPECT_TRUE(movegen->contains(Move("Xe2")));
//		EXPECT_TRUE(movegen->contains(Move("Xe4")));
//		EXPECT_TRUE(movegen->contains(Move("Xf4")));
//		EXPECT_TRUE(movegen->contains(Move("Xe6")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
////		EXPECT_EQ(movegen_caro->count(), 7); // TODO this value is if we assume that wall also blocks a five in Caro
//		EXPECT_EQ(movegen_caro->count(), 6);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe1")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe2")));
////		EXPECT_TRUE(movegen_caro->contains(Move("Xd4"))); // TODO this move is here if we assume that wall also blocks a five in Caro
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe4")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xf4")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe6")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xe7")));
//	}
//	TEST(TestMoveGenerator, DefendLossIn6_3x3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ ! _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ O _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ O _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ ! ! O O ! _ _\n" /*  5 */
//					/*  6 */" _ _ _ ! _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 5);
//		EXPECT_TRUE(movegen->contains(Move("Xd2")));
//		EXPECT_TRUE(movegen->contains(Move("Xc5")));
//		EXPECT_TRUE(movegen->contains(Move("Xd5")));
//		EXPECT_TRUE(movegen->contains(Move("Xg5")));
//		EXPECT_TRUE(movegen->contains(Move("Xd6")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//		EXPECT_EQ(movegen_caro->count(), 9);
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd1")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd2")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xb5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xc5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xg5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xh5")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd6")));
//		EXPECT_TRUE(movegen_caro->contains(Move("Xd7")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v1)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ ! _ _\n" /*  2 */
//					/*  3 */" _ X O O O ! ! X _\n" /*  3 */
//					/*  4 */" _ _ _ _ O _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ O _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ ! _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 4);
//		EXPECT_TRUE(movegen->contains(Move("Xg2")));
//		EXPECT_TRUE(movegen->contains(Move("Xf3")));
//		EXPECT_TRUE(movegen->contains(Move("Xg3")));
//		EXPECT_TRUE(movegen->contains(Move("Xc6")));
//
//		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
//		movegen_caro->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_caro->isInDanger());
//		EXPECT_FALSE(movegen_caro->isThreatening());
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ ! _ _\n" /*  2 */
//					/*  3 */" _ X O O O ! ! X _\n" /*  3 */
//					/*  4 */" _ _ _ _ O _ ! _ _\n" /*  4 */
//					/*  5 */" _ _ _ O _ _ X _ _\n" /*  5 */
//					/*  6 */" _ _ ! _ _ _ X _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ X _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ O _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 5);
//		EXPECT_TRUE(movegen->contains(Move("Xg2")));
//		EXPECT_TRUE(movegen->contains(Move("Xf3")));
//		EXPECT_TRUE(movegen->contains(Move("Xg3")));
//		EXPECT_TRUE(movegen->contains(Move("Xg4")));
//		EXPECT_TRUE(movegen->contains(Move("Xc6")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ ! _ _\n" /*  2 */
//					/*  3 */" _ X O O O ! ! X _\n" /*  3 */
//					/*  4 */" _ _ _ _ O _ ! _ _\n" /*  4 */
//					/*  5 */" _ _ _ O _ _ ! _ _\n" /*  5 */
//					/*  6 */" _ _ ! _ _ _ X _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ X _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ O _ _\n" /*  8 */
//					/*        a b c d e f g h i          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 6);
//		EXPECT_TRUE(movegen->contains(Move("Xg2")));
//		EXPECT_TRUE(movegen->contains(Move("Xf3")));
//		EXPECT_TRUE(movegen->contains(Move("Xg3")));
//		EXPECT_TRUE(movegen->contains(Move("Xg4")));
//		EXPECT_TRUE(movegen->contains(Move("Xg5")));
//		EXPECT_TRUE(movegen->contains(Move("Xc6")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v4)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i j          */
//					/*  0 */" _ _ _ _ _ _ ! _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ ! ! _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ ! ! _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ O _ O ! _ _ _\n" /*  3 */
//					/*  4 */" _ _ O _ X O _ X _ _\n" /*  4 */
//					/*  5 */" _ ! _ _ _ O _ _ X _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ X _ _ _ !\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/*        a b c d e f g h i j          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 8);
//		EXPECT_TRUE(movegen->contains(Move("Xg0")));
//		EXPECT_TRUE(movegen->contains(Move("Xe1")));
//		EXPECT_TRUE(movegen->contains(Move("Xf1")));
//		EXPECT_TRUE(movegen->contains(Move("Xe2")));
//		EXPECT_TRUE(movegen->contains(Move("Xf2")));
//		EXPECT_TRUE(movegen->contains(Move("Xg3")));
//		EXPECT_TRUE(movegen->contains(Move("Xb5")));
//		EXPECT_TRUE(movegen->contains(Move("Xj6")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v5)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i j          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ X ! O O ! O ! _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ O _ _ _ _\n" /*  5 */
//					/*  6 */" _ _ _ _ _ O _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ _ ! _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//					/*        a b c d e f g h i j          */);
//// @formatter:on
//
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xc4")));
//		EXPECT_TRUE(movegen->contains(Move("Xf4")));
//		EXPECT_TRUE(movegen->contains(Move("Xh4")));
//	}
//
//	TEST(TestMoveGenerator, GamePosition1)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ O _\n" /*  3 */
//					/*  4 */" _ _ _ _ _ _ _ _ ! _ _ _ X _ X\n" /*  4 */
//					/*  5 */" _ _ _ _ _ _ _ _ _ _ O X _ _ X\n" /*  5 */
//					/*  6 */" _ _ _ _ _ _ _ _ _ _ X X _ _ X\n" /*  6 */
//					/*  7 */" _ _ _ _ _ _ _ _ _ _ O X _ _ O\n" /*  7 */
//					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ O O _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O\n" /*  9 */
//					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xi4")));
//	}
//	TEST(TestMoveGenerator, GamePosition2)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ _ _ _ X _ O _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */" _ X O _ O _ X _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ O X X X O X _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ O X O X ! O _ _ O _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ O O X X _ X ! _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ O _ X _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ X X X O O _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 2);
//		EXPECT_TRUE(movegen->contains(Move("Xf8")));
//		EXPECT_TRUE(movegen->contains(Move("Xj9")));
//
//		MoveGenWrapper movegen_renju(GameRules::RENJU, board, Sign::CROSS);
//		movegen_renju->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen_renju->isInDanger());
//		EXPECT_FALSE(movegen_renju->isThreatening());
//		EXPECT_GE(movegen_renju->count(), 2);
//		EXPECT_FALSE(movegen_renju->contains(Move("Xf8")));
//		EXPECT_FALSE(movegen_renju->contains(Move("Xj9")));
//	}
//	TEST(TestMoveGenerator, GamePosition3)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i j k l m n o          */
//					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ O _ O _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ O _ X _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ X _ X _ _ _ ! _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ X O X O X _ ! _ _ _ _ _\n" /*  4 */
//					/*  5 */" _ _ X _ O O X _ X _ ! _ _ _ _\n" /*  5 */
//					/*  6 */" _ O _ O X _ X X O O _ _ _ _ _\n" /*  6 */
//					/*  7 */" _ _ _ _ O _ X _ O _ _ _ _ _ _\n" /*  7 */
//					/*  8 */" _ _ _ _ _ O _ ! _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  9 */
//					/* 10 */" _ _ _ _ _ ! _ X _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 5);
//		EXPECT_TRUE(movegen->contains(Move("Xk3")));
//		EXPECT_TRUE(movegen->contains(Move("Xj4")));
//		EXPECT_TRUE(movegen->contains(Move("Xk5")));
//		EXPECT_TRUE(movegen->contains(Move("Xh8")));
//		EXPECT_TRUE(movegen->contains(Move("Xf10")));
//
////		MoveGenWrapper movegen_caro(GameRules::CARO, board, Sign::CROSS);
////		movegen_caro->generate(MoveGeneratorMode::NORMAL);
////		EXPECT_FALSE(movegen_caro->isInDanger());
////		EXPECT_FALSE(movegen_caro->isThreatening());
////		EXPECT_GE(movegen_caro->count(), 2);
////		EXPECT_FALSE(movegen_caro->contains(Move("Xf8")));
////		EXPECT_FALSE(movegen_caro->contains(Move("Xj9")));
//	}
//	TEST(TestMoveGenerator, GamePosition4)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o p q r s t*/
//				        /* 0 */ " O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ O _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ ! O _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ X X _ O _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ X O O _ X _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ O ! X ! _ X O _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 0 */ " _ _ O _ _ _ _ O _ O _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 3);
//		EXPECT_TRUE(movegen->contains(Move("Xm5")));
//		EXPECT_TRUE(movegen->contains(Move("Xh8")));
//		EXPECT_TRUE(movegen->contains(Move("Xj8")));
//	}
//	TEST(TestMoveGenerator, GamePosition5)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o p q r s t*/
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ X _ O _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ X ! X _ _ _ O\n"
//				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ O _ X _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ O X X X O X _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ X O O O X _ X O\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ O _ _ X X O O _ O\n"
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ O X O _ _ O\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X _ _ X\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_EQ(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xo5")));
//	}
//	TEST(TestMoveGenerator, GamePosition6)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o p q r s t*/
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " X O O O O X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ X O X X O X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ X O O O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ O _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ X O X X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ ! O O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xc7")));
//	}
//	TEST(TestMoveGenerator, GamePosition7)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o */
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ X O _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ O X _ X _ _ _ _\n"
//				        /* 8 */ " _ _ _ X _ X _ O O O X _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ O X X X O O O X _ _ _\n"
//				        /*10 */ " _ _ _ _ _ O _ O X X _ _ O _ _\n"
//				        /*11 */ " _ _ _ _ _ _ O X O O _ _ X _ _\n"
//				        /*12 */ " _ _ _ _ _ X X _ _ ! X O _ _ _\n"
//				        /*13 */ " _ _ _ _ _ O _ _ _ _ O O _ _ _\n"
//				        /*14 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
//						/*        a b c d e f g h i j k l m n o */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xj12")));
//	}
//	TEST(TestMoveGenerator, GamePosition8)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o*/
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ X X O O O O X _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ O O X X X O ! _ X _ _ _\n"
//				        /* 6 */ " _ _ _ X X O O X X X O _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ O O O X O O _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ X _ O _ X _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ X _ X O X _ _ _ _ _ _\n"
//				        /* 0 */ " _ _ _ _ _ _ O _ O _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ O _ X _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//						/*        a b c d e f g h i j k l m n o*/);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xj5")));
//	}
//	TEST(TestMoveGenerator, GamePosition9)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o*/
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ O _\n"
//				        /* 2 */ " _ _ _ _ _ X O _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ X X O _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ O O X _ O _ X _ _\n"
//				        /* 5 */ " _ _ _ _ _ ! O _ X X X O _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ X _ _ _ O O X _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ X _ X O _ O _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ _\n"
//				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//						/*        a b c d e f g h i j k l m n o*/);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xf5")));
//	}
//	TEST(TestMoveGenerator, GamePosition10)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//						/*        a b c d e f g h i j k l m n o p q r s t*/
//				        /* 0 */ " _ _ _ _ O _ X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " O O O X X O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " X O X X O X X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " X X X O O O O X _ O _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " X O O O O X O O X _ _ O _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " X X X O _ X X X O _ X _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " O _ O O X O X O _ X O _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ X _ X O O X X X O _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " O X X X X O O X O O _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ O _ X O _ O X O _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 0 */ " _ _ _ O _ X O X X X O _ _ _ _ _ _ _ _ _\n"
//				        /* 1 */ " _ _ _ X O _ X O _ _ X _ _ _ _ _ _ _ _ _\n"
//				        /* 2 */ " _ _ _ _ ! X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 3 */ " _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//						/*        a b c d e f g h i j k l m n o p q r s t*/);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_FALSE(movegen->isInDanger());
//		EXPECT_TRUE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xe12")));
//	}
//	TEST(TestMoveGenerator, GamePosition11)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*         a b c d e f g h i j k l m n o        */
//					/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//					/*  1 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//					/*  2 */ " _ _ O _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//					/*  3 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//					/*  4 */ " _ _ X O _ O _ _ _ _ _ _ _ _ _\n" /*  4 */
//					/*  5 */ " _ _ X O X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//					/*  6 */ " _ O O X O X _ _ _ _ _ _ _ _ _\n" /*  6 */
//					/*  7 */ " _ _ X X ! O X _ _ _ _ _ _ _ _\n" /*  7 */
//					/*  8 */ " _ _ _ X _ X O O _ _ _ _ _ _ _\n" /*  8 */
//					/*  9 */ " _ _ _ _ X _ O X O _ _ _ _ _ _\n" /*  9 */
//					/* 10 */ " _ _ _ O _ O _ _ _ _ _ _ _ _ _\n" /* 10 */
//					/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//					/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//					/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//					/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//					/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xe7")));
//	}
//	TEST(TestMoveGenerator, GamePosition12)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//				/*         a b c d e f g h i j k l m n o        */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ O _ _ _ _ X _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " O _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ O O _ X _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ O O _ ! _ _ X _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ X _ _ _ _ X _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Xe8")));
//	}
//	TEST(TestMoveGenerator, GamePosition12)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ _ O O X _ ! _ _ X X\n" /*  8 */
//				/*  9 */ " _ _ _ _ O X X X X O X _ O O _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ O X O _ X X X O _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ O _ X X O _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ X O X X _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ O X O _ O _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CIRCLE);
//		movegen->generate(MoveGeneratorMode::NORMAL);
//		EXPECT_TRUE(movegen->isInDanger());
//		EXPECT_FALSE(movegen->isThreatening());
//		EXPECT_GE(movegen->count(), 1);
//		EXPECT_TRUE(movegen->contains(Move("Ok8")));
//	}
//
////	TEST(TestMoveGenerator, Placeholder15x15)
////	{
////		// @formatter:off
////		matrix<Sign> board = Board::fromString(
////					/*        a b c d e f g h i j k l m n o          */
////					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
////					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
////					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
////					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
////					/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
////					/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
////					/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
////					/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
////					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
////					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
////					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
////					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
////					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
////					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
////					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
////					/*        a b c d e f g h i j k l m n o          */);
////// @formatter:on
////
////		EXPECT_TRUE(true);
////	}
////	TEST(TestMoveGenerator, Placeholder9x9)
////	{
////		// @formatter:off
////		matrix<Sign> board = Board::fromString(
////					/*        a b c d e f g h i          */
////					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
////					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
////					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
////					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
////					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
////					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
////					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
////					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
////					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
////					/*        a b c d e f g h i          */);
////// @formatter:on
////
////		EXPECT_TRUE(true);
////	}
////	TEST(TestMoveGenerator, Placeholder5x5)
////	{
////		// @formatter:off
////		matrix<Sign> board = Board::fromString(
////					/*        a b c d e          */
////					/*  0 */" _ _ _ _ _\n" /*  0 */
////					/*  1 */" _ _ _ _ _\n" /*  1 */
////					/*  2 */" _ _ _ _ _\n" /*  2 */
////					/*  3 */" _ _ _ _ _\n" /*  3 */
////					/*  4 */" _ _ _ _ _\n" /*  4 */
////					/*        a b c d e          */);
////// @formatter:on
////
////		EXPECT_TRUE(true);
////	}
//} /* namespace ag */

