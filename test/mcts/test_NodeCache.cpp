/*
 * test_NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <gtest/gtest.h>

namespace
{
	ag::Board getRandomBoard(ag::GameConfig cfg)
	{
		ag::Board result(cfg);
		for (int i = 0; i < result.rows(); i++)
			for (int j = 0; j < result.cols(); j++)
				if (ag::randInt(4) == 0)
					result.at(i, j) = static_cast<ag::Sign>(ag::randInt(1, 3));
		return result;
	}
	class TestNodeCache: public ::testing::Test
	{
		protected:
			const ag::GameConfig game_config;
			ag::NodeCache cache;
			const ag::Board board;

			TestNodeCache() :
					game_config(ag::GameRules::STANDARD, 4, 5),
					cache(game_config),
					board(getRandomBoard(game_config))
			{
			}
	};
}

namespace ag
{
	TEST_F(TestNodeCache, init)
	{
		EXPECT_EQ(cache.allocatedElements(), 0);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.numberOfBins(), (1 << 10));
	}
	TEST_F(TestNodeCache, contains)
	{
		bool flag = cache.contains(board);
		EXPECT_FALSE(flag);
	}
	TEST_F(TestNodeCache, seek_not_in_cache)
	{
		Node *node = cache.seek(board);
		EXPECT_NE(node, nullptr);
		EXPECT_EQ(cache.storedElements(), 1);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 0);
	}
	TEST_F(TestNodeCache, seek_in_cache)
	{
		Node *node = cache.seek(board);
		Node *found = cache.seek(board);
		EXPECT_EQ(cache.storedElements(), 1);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(node, found);
	}
	TEST_F(TestNodeCache, remove)
	{
		[[maybe_unused]] Node *node = cache.seek(board);
		cache.remove(board);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 1);
		bool flag = cache.contains(board);
		EXPECT_FALSE(flag);
	}
	TEST_F(TestNodeCache, rehash)
	{
		for (int i = 0; i < 1000; i++)
			cache.seek(getRandomBoard(game_config));
		cache.seek(board);
		cache.rehash(12);
		bool flag = cache.contains(board);
		EXPECT_TRUE(flag);
	}

} /* namespace ag */
