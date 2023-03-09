/*
 * test_SharedHashTable.cpp
 *
 *  Created on: Mar 9, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/SharedHashTable.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestSharedTableData, default_constructor)
	{
		SharedTableData data;

		EXPECT_FALSE(data.mustDefend());
		EXPECT_FALSE(data.hasInitiative());
		EXPECT_EQ(data.bound(), Bound::NONE);
		EXPECT_EQ(data.generation(), 0);
		EXPECT_EQ(data.depth(), 0);
		EXPECT_EQ(data.score(), Score());
		EXPECT_EQ(data.move(), Move());
		EXPECT_EQ(data.key(), HashKey64());
	}
	TEST(TestSharedTableData, constructor)
	{
		SharedTableData data(true, true, Bound::EXACT, 45, Score(123), Move("Oa1"));

		EXPECT_TRUE(data.mustDefend());
		EXPECT_TRUE(data.hasInitiative());
		EXPECT_EQ(data.bound(), Bound::EXACT);
		EXPECT_EQ(data.generation(), 0);
		EXPECT_EQ(data.depth(), 45);
		EXPECT_EQ(data.score(), Score(123));
		EXPECT_EQ(data.move(), Move("Oa1"));
		EXPECT_EQ(data.key(), HashKey64());
	}

	TEST(TestSharedHashTable, insert_seek)
	{
		SharedTableData data(true, true, Bound::EXACT, 45, Score(123), Move("Oa1"));

		SharedHashTable table(10, 10, 1024);
		HashKey128 hash_key;
		hash_key.init();
	}

} /* namespace ag */

