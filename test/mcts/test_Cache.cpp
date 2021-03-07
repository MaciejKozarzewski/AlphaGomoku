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
		result.sign_to_move = Sign::CROSS;
		result.setValue(randFloat());
		return result;
	}
}

namespace ag
{
	TEST(TestCache , init)
	{
		Cache cache(20, 1);
		EXPECT_EQ(cache.allocatedElements(), 0);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(cache.storedElements(), 0);
		EXPECT_EQ(cache.numberOfBins(), 1);
	}
	TEST(TestCache, insert)
	{
		Cache cache(20, 1);
		EvaluationRequest req(4, 5);
		req.getBoard().at(2, 3) = Sign::CROSS;
		req.getPolicy().at(1, 1) = 0.9f;
		req.setValue(0.54f);

		cache.insert(req);
		EXPECT_EQ(cache.allocatedElements(), 1);
		EXPECT_EQ(cache.bufferedElements(), 0);
		EXPECT_EQ(cache.storedElements(), 1);
		EXPECT_EQ(cache.numberOfBins(), 1);
	}
	TEST(TestCache, seek)
	{
		Cache cache(20, 1024);
		for (int i = 0; i < 1000; i++)
			cache.insert(getRandomRequest(4, 5));

		EvaluationRequest req = getRandomRequest(4, 5);
		cache.insert(req);
		matrix<float> policy = req.getPolicy();
		float value = req.getValue();

		req.getPolicy().clear();
		req.setValue(0.0f);

		bool flag = cache.seek(req);
		EXPECT_TRUE(flag);
		EXPECT_FLOAT_EQ(req.getValue(), value);
		for (int i = 0; i < policy.size(); i++)
			EXPECT_NEAR(req.getPolicy().data()[i], policy.data()[i], 0.0001f);
	}
	TEST(TestCache, remove)
	{
		Cache cache(20, 1024);
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
		Cache cache(20, 1024);
		for (int i = 0; i < 1000; i++)
			cache.insert(getRandomRequest(4, 5));

		EvaluationRequest req = getRandomRequest(4, 5);
		cache.insert(req);

		cache.rehash(65536);

		bool flag = cache.seek(req);
		EXPECT_TRUE(flag);
	}
}

