/*
 * test_ZobristHashing.cpp
 *
 *  Created on: Sep 13, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/game/Board.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestFullZobristHashing, getHash)
	{
// @formatter:off
		const matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
				    /*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on
		const FullZobristHashing hashing(board.rows(), board.cols());

		const HashKey64 hash_cross = hashing.getHash(board, Sign::CROSS);
		const HashKey64 hash_circle = hashing.getHash(board, Sign::CIRCLE);
		EXPECT_NE(hash_cross, hash_circle);
	}

	TEST(TestFastZobristHashing, add)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ _\n" /*  3 */
					/*  4 */" _ _ _ _ _\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		const FastZobristHashing hashing(board.rows(), board.cols());

		HashKey128 hash0 = hashing.getHash(board);

		const Move move1("Oe3");
		hashing.updateHash(hash0, move1);
		Board::putMove(board, move1);
		HashKey128 hash1 = hashing.getHash(board);
		EXPECT_EQ(hash0, hash1);

		const Move move2("Xe4");
		hashing.updateHash(hash0, move2);
		Board::putMove(board, move2);
		HashKey128 hash2 = hashing.getHash(board);
		EXPECT_EQ(hash0, hash2);
	}
	TEST(TestFastZobristHashing, undo)
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
					/*        a b c d e          */
					/*  0 */" _ O _ _ _\n" /*  0 */
					/*  1 */" _ _ _ _ _\n" /*  1 */
					/*  2 */" _ X _ _ _\n" /*  2 */
					/*  3 */" _ _ _ _ O\n" /*  3 */
					/*  4 */" _ _ _ _ X\n" /*  4 */
					/*        a b c d e          */);
// @formatter:on

		const FastZobristHashing hashing(board.rows(), board.cols());

		HashKey128 hash0 = hashing.getHash(board);

		const Move move1("Oe3");
		hashing.updateHash(hash0, move1);
		Board::undoMove(board, move1);
		HashKey128 hash1 = hashing.getHash(board);
		EXPECT_EQ(hash0, hash1);

		const Move move2("Xe4");
		hashing.updateHash(hash0, move2);
		Board::undoMove(board, move2);
		HashKey128 hash2 = hashing.getHash(board);
		EXPECT_EQ(hash0, hash2);
	}

} /* namespace ag */

