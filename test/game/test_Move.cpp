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
	TEST(TestMove, init)
	{
		Move move(10, 11, Sign::CIRCLE);

		uint16_t tmp = move.toShort();

		Move loaded(tmp);
		EXPECT_EQ(move, loaded);
		EXPECT_EQ(Move::getRow(tmp), 10);
		EXPECT_EQ(Move::getCol(tmp), 11);
		EXPECT_EQ(Move::getSign(tmp), Sign::CIRCLE);
	}
	TEST(TestMove, from_text)
	{
		EXPECT_EQ(Move(0, 0, Sign::CROSS), Move::fromText("Xa0"));
		EXPECT_EQ(Move(0, 0, Sign::CIRCLE), Move::fromText("Oa0"));
		EXPECT_EQ(Move(0, 6, Sign::CROSS), Move::fromText("Xg0"));
		EXPECT_EQ(Move(7, 1, Sign::CIRCLE), Move::fromText("Ob7"));
		EXPECT_EQ(Move(11, 2, Sign::CROSS), Move::fromText("Xc11"));
	}
}

