/*
 * test_NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/NodeCache.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <gtest/gtest.h>

namespace
{
//	using namespace ag;
//	matrix<Sign> getRandomBoard(GameConfig cfg)
//	{
//		matrix<Sign> result(cfg.rows, cfg.cols);
//		for (int i = 0; i < result.rows(); i++)
//			for (int j = 0; j < result.cols(); j++)
//				if (randInt(4) == 0)
//					result.at(i, j) = static_cast<Sign>(randInt(1, 3));
//		return result;
//	}
//	class TestNodeCache: public ::testing::Test
//	{
//		protected:
//			const GameConfig game_config;
//			NodeCache cache;
//			ZobristHashing hashing;
//			const matrix<Sign> board;
//			const Sign sign_to_move;
//
//			TestNodeCache() :
//					game_config(GameRules::STANDARD, 4, 5),
//					cache(game_config),
//					hashing(game_config),
//					board(getRandomBoard(game_config)),
//					sign_to_move(Sign::CROSS)
//			{
//			}
//	};
}

namespace ag
{
//	TEST_F(TestNodeCache, init)
//	{
//		EXPECT_EQ(cache.allocatedElements(), 0);
//		EXPECT_EQ(cache.bufferedElements(), 0);
//		EXPECT_EQ(cache.storedElements(), 0);
//		EXPECT_EQ(cache.numberOfBins(), (1 << 10));
//	}
//	TEST_F(TestNodeCache, seek_not_in_cache)
//	{
//		Node *node = cache.seek(board, sign_to_move);
//		EXPECT_EQ(node, nullptr);
//		EXPECT_EQ(cache.storedElements(), 0);
//		EXPECT_EQ(cache.allocatedElements(), 0);
//		EXPECT_EQ(cache.bufferedElements(), 0);
//	}
//	TEST_F(TestNodeCache, seek_in_cache)
//	{
//		Node *node = cache.insert(board, sign_to_move);
//		Node *found = cache.seek(board, sign_to_move);
//		EXPECT_EQ(cache.storedElements(), 1);
//		EXPECT_EQ(cache.allocatedElements(), 1);
//		EXPECT_EQ(cache.bufferedElements(), 0);
//		EXPECT_EQ(node, found);
//	}
//	TEST_F(TestNodeCache, remove)
//	{
//		[[maybe_unused]] Node *node = cache.insert(board, sign_to_move);
//		cache.remove(board, sign_to_move);
//		EXPECT_EQ(cache.storedElements(), 0);
//		EXPECT_EQ(cache.allocatedElements(), 1);
//		EXPECT_EQ(cache.bufferedElements(), 1);
//		node = cache.seek(board, sign_to_move);
//		EXPECT_EQ(node, nullptr);
//	}
//	TEST_F(TestNodeCache, resize)
//	{
//		for (int i = 0; i < 1000; i++)
//		{
//			matrix<Sign> b = getRandomBoard(game_config);
//			if (b != board and cache.seek(b, sign_to_move) == nullptr)
//				cache.insert(b, sign_to_move);
//		}
//		cache.insert(board, sign_to_move);
//		cache.resize(12);
//		Node *node = cache.seek(board, sign_to_move);
//		EXPECT_NE(node, nullptr);
//	}

} /* namespace ag */

