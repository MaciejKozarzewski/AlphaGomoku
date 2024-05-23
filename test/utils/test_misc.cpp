/*
 * test_misc.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Board.hpp>

#include <gtest/gtest.h>

namespace ag
{

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

	TEST(TestMisc, sfill_unsigned)
	{
		const std::string str_5 = sfill(5, 3, false);
		const std::string str_15 = sfill(15, 3, false);
		const std::string str_125 = sfill(125, 3, false);
		const std::string str_12345 = sfill(12345, 8, false);

		EXPECT_EQ(str_5, "  5");
		EXPECT_EQ(str_15, " 15");
		EXPECT_EQ(str_125, "125");
		EXPECT_EQ(str_12345, "   12345");
	}
	TEST(TestMisc, sfill_signed)
	{
		const std::string str_5 = sfill(5, 3, true);
		const std::string str_15 = sfill(-15, 3, true);
		const std::string str_125 = sfill(-125, 3, true);
		const std::string str_123 = sfill(123, 3, true);
		const std::string str_12345 = sfill(-12345, 8, true);

		EXPECT_EQ(str_5, " +5");
		EXPECT_EQ(str_15, "-15");
		EXPECT_EQ(str_125, "-125");
		EXPECT_EQ(str_123, "+123");
		EXPECT_EQ(str_12345, "  -12345");
	}

} /* namespace ag */

