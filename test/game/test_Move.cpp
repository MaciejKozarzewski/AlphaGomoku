/*
 * test_Move.cpp
 *
 *  Created on: Mar 3, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestMove, from_text)
	{
		EXPECT_EQ(Move(0, 0, Sign::CROSS), Move("Xa0"));
		EXPECT_EQ(Move(0, 0, Sign::CIRCLE), Move("Oa0"));
		EXPECT_EQ(Move(0, 6, Sign::CROSS), Move("Xg0"));
		EXPECT_EQ(Move(7, 1, Sign::CIRCLE), Move("Ob7"));
		EXPECT_EQ(Move(11, 2, Sign::CROSS), Move("Xc11"));
	}
	TEST(TestMove, to_text)
	{
		EXPECT_EQ(Move(0, 0, Sign::CROSS).text(), "Xa0");
		EXPECT_EQ(Move(0, 0, Sign::CIRCLE).text(), "Oa0");
		EXPECT_EQ(Move(0, 6, Sign::CROSS).text(), "Xg0");
		EXPECT_EQ(Move(7, 1, Sign::CIRCLE).text(), "Ob7");
		EXPECT_EQ(Move(11, 2, Sign::CROSS).text(), "Xc11");
	}
}

