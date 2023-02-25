/*
 * test_ZobristHashing.cpp
 *
 *  Created on: Sep 13, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <gtest/gtest.h>

namespace ag
{
//	TEST(TestZobristHashing, update_add)
//	{
//		// @formatter:off
//		Board board(/*        a b c d e          */
//					/*  0 */" _ O _ _ _\n" /*  0 */
//				    /*  1 */" _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ X _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ _\n" /*  3 */
//					/*  4 */" _ _ _ _ _\n" /*  4 */
//					/*        a b c d e          */, Sign::CROSS, GameRules::STANDARD); // @formatter:on
//
//		Move m(3, 4, Sign::CROSS);
//
//		ZobristHashing hashing(GameConfig(GameRules::FREESTYLE, 5, 5));
//
//		uint64_t hash0 = hashing.getHash(board);
//		hashing.updateHash(hash0, board, m);
//
//		board.putMove(m);
//		uint64_t hash1 = hashing.getHash(board);
//		EXPECT_EQ(hash0, hash1);
//	}
//	TEST(TestZobristHashing, update_remove)
//	{
//		// @formatter:off
//		Board board(/*        a b c d e          */
//					/*  0 */" _ O _ _ _\n" /*  0 */
//					/*  1 */" _ _ _ _ _\n" /*  1 */
//					/*  2 */" _ X _ _ _\n" /*  2 */
//					/*  3 */" _ _ _ _ O\n" /*  3 */
//					/*  4 */" _ _ _ _ _\n" /*  4 */
//					/*        a b c d e          */, Sign::CROSS, GameRules::STANDARD); // @formatter:on
//
//		Move m(3, 4, Sign::CIRCLE);
//
//		ZobristHashing hashing(GameConfig(GameRules::FREESTYLE, 5, 5));
//
//		uint64_t hash0 = hashing.getHash(board);
//		hashing.updateHash(hash0, board, m);
//
//		board.undoMove(m);
//		uint64_t hash1 = hashing.getHash(board);
//
//		EXPECT_EQ(hash0, hash1);
//	}
} /* namespace ag */

