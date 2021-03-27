/*
 * test_Cache.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <gtest/gtest.h>

namespace
{
	using namespace ag;
	EvaluationRequest getRandomRequest(int rows, int cols)
	{
		EvaluationRequest result(rows, cols);
		for (int i = 0; i < rows; i++)
			for (int j = 0; j < cols; j++)
			{
				if (randInt(4) == 0)
					result.getBoard().at(i, j) = static_cast<Sign>(randInt(1, 3));
				else
					result.getPolicy().at(i, j) = randFloat();
			}
		normalize(result.getPolicy());
		result.setLastMove( { 0, 0, Sign::CROSS });
		result.setValue(Value(randFloat(), 0.0f, 0.0f));
		return result;
	}

	GameConfig getGameConfig()
	{
		GameConfig result;
		result.rules = GameRules::STANDARD;
		result.rows = 4;
		result.cols = 5;
		return result;
	}
	CacheConfig getCacheConfig(int numberOfBins)
	{
		CacheConfig result;
		result.min_cache_size = numberOfBins;
		result.max_cache_size = numberOfBins;
		result.update_from_search = false;
		result.update_visit_treshold = 10;
		return result;
	}
}

namespace ag
{
	TEST(TestCache , init)
	{
		Cache cache(getGameConfig(), getCacheConfig(1));
		EXPECT_EQ(cache.allocatedElements(), 0);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.numberOfBins(), 1);
	}
	TEST(TestCache, insert)
	{
		Cache cache(getGameConfig(), getCacheConfig(1));
		EvaluationRequest req(4, 5);
		req.getBoard().at(2, 3) = Sign::CROSS;
		req.getPolicy().at(1, 1) = 0.9f;
		req.setValue(Value(0.54f));

		cache.insert(req);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(cache.storedElements(), 1);
		EXPECT_EQ(cache.numberOfBins(), 1);
	}
	TEST(TestCache, seek)
	{
		Cache cache(getGameConfig(), getCacheConfig(1024));
		for (int i = 0; i < 1000; i++)
			cache.insert(getRandomRequest(4, 5));

		EvaluationRequest req = getRandomRequest(4, 5);
		cache.insert(req);
		matrix<float> policy = req.getPolicy();
		Value value = req.getValue();

		req.getPolicy().clear();
		req.setValue(Value());

		bool flag = cache.seek(req);
		EXPECT_TRUE(flag);
		EXPECT_FLOAT_EQ(req.getValue().win, value.win);
		for (int i = 0; i < policy.size(); i++)
			EXPECT_NEAR(req.getPolicy().data()[i], policy.data()[i], 0.0001f);
	}
	TEST(TestCache, remove)
	{
		Cache cache(getGameConfig(), getCacheConfig(1024));
		for (int i = 0; i < 1000; i++)
			cache.insert(getRandomRequest(4, 5));

		EvaluationRequest req = getRandomRequest(4, 5);
		cache.insert(req);
		cache.remove(req.getBoard(), req.getSignToMove());

		bool flag = cache.seek(req);
		EXPECT_FALSE(flag);
	}
	TEST(TestCache, rehash)
	{
		Cache cache(getGameConfig(), getCacheConfig(1024));
		for (int i = 0; i < 1000; i++)
			cache.insert(getRandomRequest(4, 5));

		EvaluationRequest req = getRandomRequest(4, 5);
		cache.insert(req);

		cache.rehash(65536);

		bool flag = cache.seek(req);
		EXPECT_TRUE(flag);
	}
	TEST(TestCache, cleanup)
	{
		Cache cache(getGameConfig(), getCacheConfig(1024));
		std::unique_ptr<Node[]> children_node1 = std::make_unique<Node[]>(12);
		std::unique_ptr<Node[]> children_node2 = std::make_unique<Node[]>(5);
		Node node1, node2;

		node1.createChildren(children_node1.get(), 12);
		node2.createChildren(children_node2.get(), 5);

	}
}

