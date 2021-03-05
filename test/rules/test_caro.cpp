/*
 * test_caro.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include "test_rules.hpp"
#include <alphagomoku/rules/caro.hpp>

#include <gtest/gtest.h>

namespace
{
	std::vector<std::string> caro_winning_patterns = { "XXXXX__", "_XXXXX_", "__XXXXX", "XXXXXX_", "_XXXXXX", "XXXXXXX", "XXXXXO_", "XXXXX_O",
				"_XXXXXO", "OXXXXX_", "O_XXXXX", "_OXXXXX", "XXXXXXO", "OXXXXXX" };
}
namespace ag
{
//	TEST(TestRules, caroFullBoard)
//	{
//		std::vector<int> cross_winning_patterns, circle_winning_patterns;
//		for (size_t i = 0; i < caro_winning_patterns.size(); i++)
//		{
//			cross_winning_patterns.push_back(testing::patternFromString(caro_winning_patterns[i]));
//			circle_winning_patterns.push_back(testing::patternFromString(testing::invert_pattern_string(caro_winning_patterns[i])));
//		}
//
//		testing::PatternGenerator generator(GameRules::CARO);
//		matrix<Sign> board(15, 15);
//		for (int i = 0; i < generator.numberOfPatterns(); i++)
//			for (int r = 0; r < board.rows() - generator.patternLength(); r++)
//				for (int c = 0; c < board.cols() - generator.patternLength(); c++)
//					for (int dir = 0; dir < 4; dir++)
//					{
//						board.clear();
//						generator.applyPattern(i, static_cast<testing::Direction>(dir), board, r, c);
//						Sign winner = getWhoWinsCaro(board);
//						if (winner == Sign::NONE)
//						{
//							EXPECT_FALSE(testing::is_winning(i, cross_winning_patterns));
//							EXPECT_FALSE(testing::is_winning(i, circle_winning_patterns));
//						}
//						if (winner == Sign::CROSS)
//						{
//							EXPECT_TRUE(testing::is_winning(i, cross_winning_patterns));
//						}
//						if (winner == Sign::CIRCLE)
//						{
//							EXPECT_TRUE(testing::is_winning(i, circle_winning_patterns));
//						}
//					}
//	}
}



