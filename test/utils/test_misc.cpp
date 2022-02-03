/*
 * test_misc.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/misc.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestMisc, isBoardFull)
	{
		matrix<Sign> board(9, 11);
		EXPECT_FALSE(isBoardFull(board));
		board.fill(Sign::CROSS);
		EXPECT_TRUE(isBoardFull(board));
		board.at(4, 5) = Sign::NONE;
		EXPECT_FALSE(isBoardFull(board));
	}
	TEST(TestMisc, isBoardEmpty)
	{
		matrix<Sign> board(9, 11);
		EXPECT_TRUE(isBoardEmpty(board));
		board.at(4, 5) = Sign::CROSS;
		EXPECT_FALSE(isBoardEmpty(board));
		board.fill(Sign::CROSS);
		EXPECT_FALSE(isBoardEmpty(board));
	}

	TEST(TestMisc, split)
	{
		std::string line = "INFO max_memory 83886080";

		auto tmp = split(line, ' ');
		EXPECT_EQ(tmp.size(), 3u);
		EXPECT_EQ(tmp[0], "INFO");
		EXPECT_EQ(tmp[1], "max_memory");
		EXPECT_EQ(tmp[2], "83886080");

		line = "";
		tmp = split(line, ' ');
		EXPECT_EQ(tmp.size(), 0u);

		line = "INFO";
		tmp = split(line, ' ');
		EXPECT_EQ(tmp.size(), 1u);
		EXPECT_EQ(tmp[0], "INFO");
	}

} /* namespace ag */

