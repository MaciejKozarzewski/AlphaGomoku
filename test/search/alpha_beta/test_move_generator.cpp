/*
 * test_move_generator.cpp
 *
 *  Created on: Oct 2, 2022
 *		Author: Mira Fontan
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>

#include <gtest/gtest.h>

namespace
{
	using namespace ag;

	class MoveGenWrapper
	{
			GameConfig game_config;
			PatternCalculator pattern_calculator;
			MoveGenerator move_generator;
			ActionStack action_stack;
		public:
			MoveGenWrapper(GameRules rules, const matrix<ag::Sign> &board, Sign signToMove) :
					game_config(rules, board.rows(), board.cols()),
					pattern_calculator(game_config),
					move_generator(game_config, pattern_calculator),
					action_stack(1024)
			{
				pattern_calculator.setBoard(board, signToMove);
			}
			ActionList operator()(MoveGeneratorMode mode)
			{
				ActionList result = action_stack.create_root();
				move_generator.generate(result, mode);
				return result;
			}
	};
}

namespace ag
{
	TEST(TestMoveGenerator, AllMoves)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ X _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen(MoveGeneratorMode::LEGAL);
		EXPECT_EQ(actions1.size(), 24);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);

		const ActionList actions2 = movegen(MoveGeneratorMode::REDUCED);
		EXPECT_EQ(actions2.size(), 24);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);

		const ActionList actions3 = movegen(MoveGeneratorMode::THREATS);
		EXPECT_EQ(actions3.size(), 0);
		EXPECT_FALSE(actions3.must_defend);
		EXPECT_FALSE(actions3.has_initiative);
	}
	TEST(TestMoveGenerator, AllMoves2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ ! ! ! !\n" /*  0 */
					/*  1 */" ! ! ! X !\n" /*  1 */
					/*  2 */" _ ! ! ! !\n" /*  2 */
					/*  3 */" _ ! ! ! !\n" /*  3 */
					/*  4 */" ! _ _ ! _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen(MoveGeneratorMode::LEGAL);
		EXPECT_EQ(actions1.size(), 24);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);

		const ActionList actions2 = movegen(MoveGeneratorMode::REDUCED);
		EXPECT_EQ(actions2.size(), 18);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);
	}
	TEST(TestMoveGenerator, AllMoves3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ O _ X _ _ _\n" /*  3 */
					/*  4 */" _ _ _ X X _ _ _ _\n" /*  4 */
					/*  5 */" _ ! ! O O O ! ! _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen(MoveGeneratorMode::LEGAL);
		EXPECT_EQ(actions1.size(), 74);

		const ActionList actions2 = movegen(MoveGeneratorMode::REDUCED);
		EXPECT_EQ(actions2.size(), 68);

		const ActionList actions3 = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions3.must_defend);
		EXPECT_FALSE(actions3.has_initiative);
		EXPECT_EQ(actions3.size(), 2);
		EXPECT_TRUE(actions3.contains(Move("Xc5")));
		EXPECT_TRUE(actions3.contains(Move("Xg5")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions4 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions4.must_defend);
		EXPECT_FALSE(actions4.has_initiative);
		EXPECT_EQ(actions4.size(), 4);
		EXPECT_TRUE(actions4.contains(Move("Xb5")));
		EXPECT_TRUE(actions4.contains(Move("Xc5")));
		EXPECT_TRUE(actions4.contains(Move("Xg5")));
		EXPECT_TRUE(actions4.contains(Move("Xh5")));

		MoveGenWrapper movegen_caro6(GameRules::CARO6, board, Sign::CROSS);
		const ActionList actions5 = movegen_caro6(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions4.equals(actions5));
	}

	TEST(TestMoveGenerator, SimpleForced)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" X _ _ _ _\n" /*  0 */
					/*  1 */" X _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ _ _\n" /*  3 */
					/*  4 */" ! _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);

		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_EQ(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xa4")));
	}
	TEST(TestMoveGenerator, SimpleForced2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" O _ _ _ _\n" /*  0 */
					/*  1 */" O _ _ _ _\n" /*  1 */
					/*  2 */" O _ _ _ _\n" /*  2 */
					/*  3 */" O _ _ _ _\n" /*  3 */
					/*  4 */" ! _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);

		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_EQ(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xa4")));
	}
	TEST(TestMoveGenerator, SimpleForced3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" O _ X _ _\n" /*  0 */
					/*  1 */" O _ X _ _\n" /*  1 */
					/*  2 */" O _ X _ _\n" /*  2 */
					/*  3 */" O _ X _ _\n" /*  3 */
					/*  4 */" _ _ ! _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);

		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_EQ(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xc4")));
	}

	TEST(TestMoveGenerator, Open3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" ! O O O ! ! _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xa0")));
		EXPECT_TRUE(actions1.contains(Move("Xe0")));
		EXPECT_TRUE(actions1.contains(Move("Xf0")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);
		EXPECT_EQ(actions2.size(), 4);
		EXPECT_TRUE(actions2.contains(Move("Xa0")));
		EXPECT_TRUE(actions2.contains(Move("Xe0")));
		EXPECT_TRUE(actions2.contains(Move("Xf0")));
		EXPECT_TRUE(actions2.contains(Move("Xg0")));

		MoveGenWrapper movegen_caro6(GameRules::CARO6, board, Sign::CROSS);
		const ActionList actions3 = movegen_caro6(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions2.equals(actions3));
	}
	TEST(TestMoveGenerator, InlineFork4x4)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" O O O ! ! ! O O O\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);

		const ActionList actions1 = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xd0")));
		EXPECT_TRUE(actions1.contains(Move("Xe0")));
		EXPECT_TRUE(actions1.contains(Move("Xf0")));
	}
	TEST(TestMoveGenerator, InlineFork4x4_v2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" O O O ! ! O O ! O\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xd0")));
		EXPECT_TRUE(actions1.contains(Move("Xe0")));
		EXPECT_TRUE(actions1.contains(Move("Xh0")));

		MoveGenWrapper movegen_standard(GameRules::STANDARD, board, Sign::CROSS);
		const ActionList actions2 = movegen_standard(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);
	}
	TEST(TestMoveGenerator, InterlacedMove)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" _ _ ! O O O ! _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" X X X ! ! _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 4);
		EXPECT_TRUE(actions1.contains(Move("Xc0")));
		EXPECT_TRUE(actions1.contains(Move("Xg0")));
		EXPECT_TRUE(actions1.contains(Move("Xd6")));
		EXPECT_TRUE(actions1.contains(Move("Xe6")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions2.must_defend);
		EXPECT_TRUE(actions2.has_initiative);
		EXPECT_EQ(actions2.size(), 6);
		EXPECT_TRUE(actions2.contains(Move("Xb0")));
		EXPECT_TRUE(actions2.contains(Move("Xc0")));
		EXPECT_TRUE(actions2.contains(Move("Xg0")));
		EXPECT_TRUE(actions2.contains(Move("Xh0")));
		EXPECT_TRUE(actions2.contains(Move("Xd6")));
		EXPECT_TRUE(actions2.contains(Move("Xe6")));
	}
	TEST(TestMoveGenerator, InterlacedMoveDuplicate)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ ! _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ O _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ O _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ O _ _ _ _\n" /*  5 */
					/*  6 */" X X X ! ! _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xe2")));
		EXPECT_TRUE(actions1.contains(Move("Xd6")));
		EXPECT_TRUE(actions1.contains(Move("Xe6")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions2.must_defend);
		EXPECT_TRUE(actions2.has_initiative);
		EXPECT_EQ(actions2.size(), 5);
		EXPECT_TRUE(actions2.contains(Move("Xe1")));
		EXPECT_TRUE(actions2.contains(Move("Xe2")));
		EXPECT_TRUE(actions2.contains(Move("Xe7")));
		EXPECT_TRUE(actions2.contains(Move("Xd6")));
		EXPECT_TRUE(actions2.contains(Move("Xe6")));
	}
	TEST(TestMoveGenerator, WinIn3Moves_1)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" ! _ X X X\n" /*  0 */
					/*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" X _ _ _ _\n" /*  2 */
					/*  3 */" X _ _ _ _\n" /*  3 */
					/*  4 */" X _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 1);
		EXPECT_TRUE(actions1.contains(Move("Xa0")));

		MoveGenWrapper movegen_renju(GameRules::RENJU, board, Sign::CROSS);
		const ActionList actions2 = movegen_renju(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_TRUE(actions2.has_initiative);
		EXPECT_EQ(actions2.size(), 18);
		EXPECT_EQ(actions2.getScoreOf(Move("Xa0")), Score::loss_in(1));
	}
	TEST(TestMoveGenerator, WinIn3Moves_2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ _ O _ _\n" /*  0 */
					/*  1 */" _ _ O _ _\n" /*  1 */
					/*  2 */" _ _ O _ _\n" /*  2 */
					/*  3 */" _ _ ! _ _\n" /*  3 */
					/*  4 */" ! O ! O O\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xa4")));
		EXPECT_TRUE(actions1.contains(Move("Xc3")));
		EXPECT_TRUE(actions1.contains(Move("Xc4")));
	}
	TEST(TestMoveGenerator, WinIn3Moves_3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ ! _ _ _\n" /*  0 */
					/*  1 */" ! ! O O O\n" /*  1 */
					/*  2 */" _ O _ _ _\n" /*  2 */
					/*  3 */" _ O _ _ _\n" /*  3 */
					/*  4 */" _ O _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xb0")));
		EXPECT_TRUE(actions1.contains(Move("Xa1")));
		EXPECT_TRUE(actions1.contains(Move("Xb1")));
	}
	TEST(TestMoveGenerator, WinIn3Moves_4)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f        */
					/*  0 */" _ ! _ _ _ _\n" /*  0 */
					/*  1 */" ! ! O O O !\n" /*  1 */
					/*  2 */" _ O _ _ _ _\n" /*  2 */
					/*  3 */" _ O _ _ _ _\n" /*  3 */
					/*  4 */" _ O _ _ _ _\n" /*  4 */
					/*  5 */" _ ! _ _ _ _\n" /*  5 */
					/*        a b c d e f        */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 1);
		EXPECT_TRUE(actions1.contains(Move("Xb1")));
	}
	TEST(TestMoveGenerator, WinIn5Moves_3x3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ X _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ ! X X _ _ _\n" /*  4 */
					/*  5 */" _ _ _ X _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 1);
		EXPECT_TRUE(actions1.contains(Move("Xd4")));

		MoveGenWrapper movegen_renju(GameRules::RENJU, board, Sign::CROSS);
		const ActionList actions2 = movegen_renju(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);
		EXPECT_TRUE(actions2.contains(Move("Xd4")));
		EXPECT_EQ(actions2.getScoreOf(Move("Xd4")), Score::loss_in(1));
	}
	TEST(TestMoveGenerator, WinIn5Moves_4x3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i          */
					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ O _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ X _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ X _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ ! X X _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ X _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ O _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*        a b c d e f g h i          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 1);
		EXPECT_TRUE(actions1.contains(Move("Xd4")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_FALSE(actions2.has_initiative);
	}
//	TEST(TestMoveGenerator, DefendLossIn6_4x3)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_EQ(actions1.size(), 4);
//		EXPECT_TRUE(actions1.contains(Move("Xe2")));
//		EXPECT_TRUE(actions1.contains(Move("Xe4")));
//		EXPECT_TRUE(actions1.contains(Move("Xf4")));
//		EXPECT_TRUE(actions1.contains(Move("Xe6")));
//
//		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
//		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions2.must_defend);
//		EXPECT_FALSE(actions2.has_initiative);
////		EXPECT_EQ(actions2.size(), 7); // TODO this value is if we assume that wall also blocks a five in Caro
//		EXPECT_EQ(actions2.size(), 6);
//		EXPECT_TRUE(actions2.contains(Move("Xe1")));
//		EXPECT_TRUE(actions2.contains(Move("Xe2")));
////		EXPECT_TRUE(actions2.contains(Move("Xd4"))); // TODO this move is here if we assume that wall also blocks a five in Caro
//		EXPECT_TRUE(actions2.contains(Move("Xe4")));
//		EXPECT_TRUE(actions2.contains(Move("Xf4")));
//		EXPECT_TRUE(actions2.contains(Move("Xe6")));
//		EXPECT_TRUE(actions2.contains(Move("Xe7")));
//	}
//	TEST(TestMoveGenerator, DefendLossIn6_3x3)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_EQ(actions1.size(), 5);
//		EXPECT_TRUE(actions1.contains(Move("Xd2")));
//		EXPECT_TRUE(actions1.contains(Move("Xc5")));
//		EXPECT_TRUE(actions1.contains(Move("Xd5")));
//		EXPECT_TRUE(actions1.contains(Move("Xg5")));
//		EXPECT_TRUE(actions1.contains(Move("Xd6")));
//
//		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
//		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions2.must_defend);
//		EXPECT_FALSE(actions2.has_initiative);
//		EXPECT_EQ(actions2.size(), 9);
//		EXPECT_TRUE(actions2.contains(Move("Xd1")));
//		EXPECT_TRUE(actions2.contains(Move("Xd2")));
//		EXPECT_TRUE(actions2.contains(Move("Xb5")));
//		EXPECT_TRUE(actions2.contains(Move("Xc5")));
//		EXPECT_TRUE(actions2.contains(Move("Xd5")));
//		EXPECT_TRUE(actions2.contains(Move("Xg5")));
//		EXPECT_TRUE(actions2.contains(Move("Xh5")));
//		EXPECT_TRUE(actions2.contains(Move("Xd6")));
//		EXPECT_TRUE(actions2.contains(Move("Xd7")));
//	}

//	TEST(TestMoveGenerator, FourFreeThree_v1)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_EQ(actions1.size(), 4);
//		EXPECT_TRUE(actions1.contains(Move("Xg2")));
//		EXPECT_TRUE(actions1.contains(Move("Xf3")));
//		EXPECT_TRUE(actions1.contains(Move("Xg3")));
//		EXPECT_TRUE(actions1.contains(Move("Xc6")));
//
//		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
//		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
//		EXPECT_FALSE(actions2.must_defend);
//		EXPECT_FALSE(actions2.has_initiative);
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v2)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_EQ(actions1.size(), 5);
//		EXPECT_TRUE(actions1.contains(Move("Xg2")));
//		EXPECT_TRUE(actions1.contains(Move("Xf3")));
//		EXPECT_TRUE(actions1.contains(Move("Xg3")));
//		EXPECT_TRUE(actions1.contains(Move("Xg4")));
//		EXPECT_TRUE(actions1.contains(Move("Xc6")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v3)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_EQ(actions1.size(), 6);
//		EXPECT_TRUE(actions1.contains(Move("Xg2")));
//		EXPECT_TRUE(actions1.contains(Move("Xf3")));
//		EXPECT_TRUE(actions1.contains(Move("Xg3")));
//		EXPECT_TRUE(actions1.contains(Move("Xg4")));
//		EXPECT_TRUE(actions1.contains(Move("Xg5")));
//		EXPECT_TRUE(actions1.contains(Move("Xc6")));
//	}
//	TEST(TestMoveGenerator, FourFreeThree_v4)
//	{
//// @formatter:off
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
//		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
//		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
//		EXPECT_TRUE(actions1.must_defend);
//		EXPECT_FALSE(actions1.has_initiative);
//		EXPECT_GE(actions1.size(), 8);
//		EXPECT_TRUE(actions1.contains(Move("Xg0")));
//		EXPECT_TRUE(actions1.contains(Move("Xe1")));
//		EXPECT_TRUE(actions1.contains(Move("Xf1")));
//		EXPECT_TRUE(actions1.contains(Move("Xe2")));
//		EXPECT_TRUE(actions1.contains(Move("Xf2")));
//		EXPECT_TRUE(actions1.contains(Move("Xg3")));
//		EXPECT_TRUE(actions1.contains(Move("Xb5")));
//		EXPECT_TRUE(actions1.contains(Move("Xj6")));
//	}
	TEST(TestMoveGenerator, FourFreeThree_v5)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ X ! O O ! O ! _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ O _ _ _ _\n" /*  5 */
					/*  6 */" _ _ _ _ _ O _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ _ ! _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _\n" /*  9 */
					/*        a b c d e f g h i j          */);
// @formatter:on

		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_FALSE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 3);
		EXPECT_TRUE(actions1.contains(Move("Xc4")));
		EXPECT_TRUE(actions1.contains(Move("Xf4")));
		EXPECT_TRUE(actions1.contains(Move("Xh4")));
	}

	TEST(TestMoveGenerator, GamePosition1)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ O _\n" /*  3 */
					/*  4 */" _ _ _ _ _ _ _ _ ! _ _ _ X _ X\n" /*  4 */
					/*  5 */" _ _ _ _ _ _ _ _ _ _ O X _ _ X\n" /*  5 */
					/*  6 */" _ _ _ _ _ _ _ _ _ _ X X _ _ X\n" /*  6 */
					/*  7 */" _ _ _ _ _ _ _ _ _ _ O X _ _ O\n" /*  7 */
					/*  8 */" _ _ _ _ _ _ _ _ _ _ _ O O _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O\n" /*  9 */
					/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xi4")));
	}
	TEST(TestMoveGenerator, GamePosition2)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ _ _ _ X _ O _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */" _ X O _ O _ X _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ O X X X O X _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ O X O X ! O _ _ O _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ O O X X _ X ! _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ O _ X _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ X X X O O _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on
		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 2);
		EXPECT_TRUE(actions1.contains(Move("Xf8")));
		EXPECT_TRUE(actions1.contains(Move("Xj9")));

		MoveGenWrapper movegen_renju(GameRules::RENJU, board, Sign::CROSS);
		const ActionList actions2 = movegen_renju(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions2.must_defend);
		EXPECT_TRUE(actions2.has_initiative);
		EXPECT_GE(actions2.size(), 2);
		EXPECT_EQ(actions2.getScoreOf(Move("Xf8")), Score::loss_in(1));
		EXPECT_EQ(actions2.getScoreOf(Move("Xj9")), Score::loss_in(1));
	}
	TEST(TestMoveGenerator, GamePosition3)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e f g h i j k l m n o          */
					/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ O _ O _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */" _ _ _ O _ X _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ X _ X _ _ _ ! _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ X O X O X _ ! _ _ _ _ _\n" /*  4 */
					/*  5 */" _ _ X _ O O X _ X _ ! _ _ _ _\n" /*  5 */
					/*  6 */" _ O _ O X _ X X O O _ _ _ _ _\n" /*  6 */
					/*  7 */" _ _ _ _ O _ X _ O _ _ _ _ _ _\n" /*  7 */
					/*  8 */" _ _ _ _ _ O _ ! _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  9 */
					/* 10 */" _ _ _ _ _ ! _ X _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*        a b c d e f g h i j k l m n o          */);
// @formatter:on
		MoveGenWrapper movegen_freestyle(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions1 = movegen_freestyle(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions1.must_defend);
		EXPECT_TRUE(actions1.has_initiative);
		EXPECT_EQ(actions1.size(), 5);
		EXPECT_TRUE(actions1.contains(Move("Xk3")));
		EXPECT_TRUE(actions1.contains(Move("Xj4")));
		EXPECT_TRUE(actions1.contains(Move("Xk5")));
		EXPECT_TRUE(actions1.contains(Move("Xh8")));
		EXPECT_TRUE(actions1.contains(Move("Xf10")));

		MoveGenWrapper movegen_caro5(GameRules::CARO5, board, Sign::CROSS);
		const ActionList actions2 = movegen_caro5(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions2.must_defend);
		EXPECT_TRUE(actions2.has_initiative);
		EXPECT_EQ(actions2.size(), 7);
		EXPECT_TRUE(actions1.contains(Move("Xk3")));
		EXPECT_TRUE(actions1.contains(Move("Xj4")));
		EXPECT_TRUE(actions2.contains(Move("Xl4")));
		EXPECT_TRUE(actions1.contains(Move("Xk5")));
		EXPECT_TRUE(actions1.contains(Move("Xh8")));
		EXPECT_TRUE(actions1.contains(Move("Xf10")));
		EXPECT_TRUE(actions2.contains(Move("Xe11")));
	}
	TEST(TestMoveGenerator, GamePosition4)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o p q r s t*/
				        /* 0 */ " O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ O _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ ! O _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ X X _ O _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ X O O _ X _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ _ _ O ! X ! _ X O _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 0 */ " _ _ O _ _ _ _ O _ O _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 3);
		EXPECT_TRUE(actions.contains(Move("Xm5")));
		EXPECT_TRUE(actions.contains(Move("Xh8")));
		EXPECT_TRUE(actions.contains(Move("Xj8")));
	}
	TEST(TestMoveGenerator, GamePosition5)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o p q r s t*/
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ X _ O _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ X ! X _ _ _ O\n"
				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ O _ X _\n"
				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ O X X X O X _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ X O O O X _ X O\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ O _ _ X X O O _ O\n"
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ O X O _ _ O\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X _ _ X\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_EQ(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xo5")));
	}
	TEST(TestMoveGenerator, GamePosition6)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o p q r s t*/
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " X O O O O X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ X O X X O X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ X O O O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ O _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ X O X X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ ! O O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xc7")));
	}
	TEST(TestMoveGenerator, GamePosition7)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o */
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ X O _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ O X _ X _ _ _ _\n"
				        /* 8 */ " _ _ _ X _ X _ O O O X _ _ _ _\n"
				        /* 9 */ " _ _ _ _ O X X X O O O X _ _ _\n"
				        /*10 */ " _ _ _ _ _ O _ O X X _ _ O _ _\n"
				        /*11 */ " _ _ _ _ _ _ O X O O _ _ X _ _\n"
				        /*12 */ " _ _ _ _ _ X X _ _ ! X O _ _ _\n"
				        /*13 */ " _ _ _ _ _ O _ _ _ _ O O _ _ _\n"
				        /*14 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
						/*        a b c d e f g h i j k l m n o */);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xj12")));
	}
	TEST(TestMoveGenerator, GamePosition8)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o*/
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ X X O O O O X _ _ _ _ _\n"
				        /* 5 */ " _ _ _ O O X X X O ! _ X _ _ _\n"
				        /* 6 */ " _ _ _ X X O O X X X O _ _ _ _\n"
				        /* 7 */ " _ _ _ _ O O O X O O _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ X _ O _ X _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ X _ X O X _ _ _ _ _ _\n"
				        /* 0 */ " _ _ _ _ _ _ O _ O _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ O _ X _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
						/*        a b c d e f g h i j k l m n o*/);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xj5")));
	}
	TEST(TestMoveGenerator, GamePosition9)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o*/
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ O _\n"
				        /* 2 */ " _ _ _ _ _ X O _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ X X O _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ O O X _ O _ X _ _\n"
				        /* 5 */ " _ _ _ _ _ ! O _ X X X O _ _ _\n"
				        /* 6 */ " _ _ _ _ _ X _ _ _ O O X _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ X _ X O _ O _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ O _ _\n"
				        /* 0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
						/*        a b c d e f g h i j k l m n o*/);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xf5")));
	}
	TEST(TestMoveGenerator, GamePosition10)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
						/*        a b c d e f g h i j k l m n o p q r s t*/
				        /* 0 */ " _ _ _ _ O _ X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " O O O X X O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " X O X X O X X _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " X X X O O O O X _ O _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " X O O O O X O O X _ _ O _ _ _ _ _ _ _ _\n"
				        /* 5 */ " X X X O _ X X X O _ X _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " O _ O O X O X O _ X O _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ X _ X O O X X X O _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " O X X X X O O X O O _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ O _ X O _ O X O _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 0 */ " _ _ _ O _ X O X X X O _ _ _ _ _ _ _ _ _\n"
				        /* 1 */ " _ _ _ X O _ X O _ _ X _ _ _ _ _ _ _ _ _\n"
				        /* 2 */ " _ _ _ _ ! X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 3 */ " _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				        /* 9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
						/*        a b c d e f g h i j k l m n o p q r s t*/);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xe12")));
	}
	TEST(TestMoveGenerator, GamePosition11)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*         a b c d e f g h i j k l m n o        */
					/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
					/*  1 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
					/*  2 */ " _ _ O _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
					/*  3 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
					/*  4 */ " _ _ X O _ O _ _ _ _ _ _ _ _ _\n" /*  4 */
					/*  5 */ " _ _ X O X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
					/*  6 */ " _ O O X O X _ _ _ _ _ _ _ _ _\n" /*  6 */
					/*  7 */ " _ _ X X ! O X _ _ _ _ _ _ _ _\n" /*  7 */
					/*  8 */ " _ _ _ X _ X O O _ _ _ _ _ _ _\n" /*  8 */
					/*  9 */ " _ _ _ _ X _ O X O _ _ _ _ _ _\n" /*  9 */
					/* 10 */ " _ _ _ O _ O _ _ _ _ _ _ _ _ _\n" /* 10 */
					/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
					/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
					/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
					/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
					/*         a b c d e f g h i j k l m n o        */);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_TRUE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xe7")));
	}
	TEST(TestMoveGenerator, GamePosition12)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l m n o        */
				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ _ O _ _ _ _ X _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  5 */
				/*  6 */ " O _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ O O _ X _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ O O _ ! _ _ X _ _ _ _ _ _ _\n" /*  8 */
				/*  9 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ X _ _ _ _ X _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /* 11 */
				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
				/*         a b c d e f g h i j k l m n o        */);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CROSS);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_FALSE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Xe8")));
	}
	TEST(TestMoveGenerator, GamePosition13)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l m n o          */
				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ _ _ _ _ O O X _ ! _ _ X X\n" /*  8 */
				/*  9 */ " _ _ _ _ O X X X X O X _ O O _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ O X O _ X X X O _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ O _ X X O _ _ _\n" /* 11 */
				/* 12 */ " _ _ _ _ _ _ _ _ _ X O X X _ _\n" /* 12 */
				/* 13 */ " _ _ _ _ _ _ _ _ O X O _ O _ _\n" /* 13 */
				/* 14 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /* 14 */
				/*         a b c d e f g h i j k l m n o          */);
// @formatter:on
		MoveGenWrapper movegen(GameRules::FREESTYLE, board, Sign::CIRCLE);
		const ActionList actions = movegen(MoveGeneratorMode::OPTIMAL);
		EXPECT_FALSE(actions.must_defend);
		EXPECT_TRUE(actions.has_initiative);
		EXPECT_GE(actions.size(), 1);
		EXPECT_TRUE(actions.contains(Move("Ok8")));
	}

//	TEST(TestMoveGenerator, Placeholder15x15)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
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
//					/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//
//		EXPECT_TRUE(true);
//	}
//	TEST(TestMoveGenerator, Placeholder9x9)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e f g h i          */
//					/*  0 */" _ _ _ _ _ _ _ _ _\n" /*  0 */
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
//		EXPECT_TRUE(true);
//	}
//	TEST(TestMoveGenerator, Placeholder5x5)
//	{
//		// @formatter:off
//		matrix<Sign> board = Board::fromString(
//					/*        a b c d e          */
//					/*  0 */" _ _ _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ _ _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _\n" /*  4 */
//					/*        a b c d e          */);
//// @formatter:on
//
//		EXPECT_TRUE(true);
//	}
} /* namespace ag */

