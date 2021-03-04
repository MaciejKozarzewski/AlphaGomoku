/*
 * test_Move.cpp
 *
 *  Created on: Mar 3, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Move.hpp>

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
}

