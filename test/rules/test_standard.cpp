/*
 * test_standard.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include "test_rules.hpp"
#include <alphagomoku/utils/game_rules.hpp>

#include <gtest/gtest.h>

namespace
{
	std::vector<std::string> standard_winning_patterns = { "XXXXX_", "_XXXXX", "OXXXXX", "XXXXXO", };
}
namespace ag
{
	TEST(TestRules, standardFullBoard)
	{
		std::vector<int> cross_winning_patterns, circle_winning_patterns;
		for (size_t i = 0; i < standard_winning_patterns.size(); i++)
		{
			cross_winning_patterns.push_back(testing::patternFromString(standard_winning_patterns[i]));
			circle_winning_patterns.push_back(testing::patternFromString(testing::invert_pattern_string(standard_winning_patterns[i])));
		}

		testing::PatternGenerator generator(GameRules::STANDARD);
		matrix<Sign> board(15, 15);
		for (int i = 0; i < generator.numberOfPatterns(); i++)
			for (int r = 0; r < board.rows() - generator.patternLength(); r++)
				for (int c = 0; c < board.cols() - generator.patternLength(); c++)
					for (int dir = 0; dir < 4; dir++)
					{
						board.clear();
						generator.applyPattern(i, static_cast<testing::Direction>(dir), board, r, c);
						Sign winner = getWhoWins(generator.rules, board);
						if (winner == Sign::NONE)
						{
							EXPECT_FALSE(testing::is_winning(i, cross_winning_patterns));
							EXPECT_FALSE(testing::is_winning(i, circle_winning_patterns));
						}
						if (winner == Sign::CROSS)
						{
							EXPECT_TRUE(testing::is_winning(i, cross_winning_patterns));
						}
						if (winner == Sign::CIRCLE)
						{
							EXPECT_TRUE(testing::is_winning(i, circle_winning_patterns));
						}
					}
	}
	TEST(TestRules, standardLastMove)
	{
		std::vector<int> cross_winning_patterns, circle_winning_patterns;
		for (size_t i = 0; i < standard_winning_patterns.size(); i++)
		{
			cross_winning_patterns.push_back(testing::patternFromString(standard_winning_patterns[i]));
			circle_winning_patterns.push_back(testing::patternFromString(testing::invert_pattern_string(standard_winning_patterns[i])));
		}

		testing::PatternGenerator generator(GameRules::STANDARD);
		matrix<Sign> board(15, 15);
		for (int i = 0; i < generator.numberOfPatterns(); i++)
			for (int r = 0; r < board.rows() - generator.patternLength(); r++)
				for (int c = 0; c < board.cols() - generator.patternLength(); c++)
					for (int dir = 0; dir < 4; dir++)
					{
						board.clear();
						generator.applyPattern(i, static_cast<testing::Direction>(dir), board, r, c);
						for (int x = r; x < r + generator.patternLength(); x++)
							for (int y = c; y < c + generator.patternLength(); y++)
							{
								if (board.at(x, y) == Sign::CROSS)
								{
									Sign winner = getWhoWins(generator.rules, board, Move(x, y, board.at(x, y)));
									Sign winner_full = getWhoWins(generator.rules, board);
									if (winner_full == Sign::CROSS)
									{
										EXPECT_EQ(winner, winner_full);
									}
								}
								if (board.at(x, y) == Sign::CIRCLE)
								{
									Sign winner = getWhoWins(generator.rules, board, Move(x, y, board.at(x, y)));
									Sign winner_full = getWhoWins(generator.rules, board);
									if (winner_full == Sign::CIRCLE)
									{
										EXPECT_EQ(winner, winner_full);
									}
								}
							}
					}
	}
}

