/*
 * test_NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <gtest/gtest.h>

namespace
{
	using namespace ag;
	Board getRandomBoard(GameConfig cfg)
	{
		Board result(cfg);
		for (int i = 0; i < result.rows(); i++)
			for (int j = 0; j < result.cols(); j++)
				if (randInt(4) == 0)
					result.at(i, j) = static_cast<Sign>(randInt(1, 3));
		return result;
	}
	class TestNodeCache: public ::testing::Test
	{
		protected:
			const GameConfig game_config;
			NodeCache cache;
			ZobristHashing hashing;
			const Board board;
			const uint64_t hash;

			TestNodeCache() :
					game_config(GameRules::STANDARD, 4, 5),
					hashing(game_config),
					board(getRandomBoard(game_config)),
					hash(hashing.getHash(board))
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
	TEST_F(TestNodeCache, seek_not_in_cache)
	{
		Node *node = cache.seek(hash);
		EXPECT_EQ(node, nullptr);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.allocatedElements(), 0);
		EXPECT_EQ(cache.bufferedElements(), 0);
	}
	TEST_F(TestNodeCache, seek_in_cache)
	{
		Node *node = cache.insert(hash);
		Node *found = cache.seek(hash);
		EXPECT_EQ(cache.storedElements(), 1);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(node, found);
	}
	TEST_F(TestNodeCache, remove)
	{
		[[maybe_unused]] Node *node = cache.insert(hash);
		cache.remove(hash);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 1);
		node = cache.seek(hash);
		EXPECT_EQ(node, nullptr);
	}
	TEST_F(TestNodeCache, resize)
	{
		for (int i = 0; i < 1000; i++)
		{
			Board b = getRandomBoard(game_config);
			if (b != board and cache.seek(hashing.getHash(b)) == nullptr)
				cache.insert(hashing.getHash(b));
		}
		cache.insert(hash);
		cache.resize(12);
		Node *node = cache.seek(hash);
		EXPECT_NE(node, nullptr);
	}

} /* namespace ag */

