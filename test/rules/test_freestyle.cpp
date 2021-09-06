/*
 * test_freestyle.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#include "test_rules.hpp"
#include <alphagomoku/rules/game_rules.hpp>

#include <gtest/gtest.h>

namespace
{
	std::vector<std::string> freestyle_winning_patterns = { "XXXXX_", "XXXXXO", "XXXXXX", "OXXXXX", "_XXXXX" };

}
namespace ag
{
	TEST(TestRules, freestyleFullBoard)
	{
		GTEST_SKIP();
		std::vector<int> cross_winning_patterns, circle_winning_patterns;
		for (size_t i = 0; i < freestyle_winning_patterns.size(); i++)
		{
			cross_winning_patterns.push_back(testing::patternFromString(freestyle_winning_patterns[i]));
			circle_winning_patterns.push_back(testing::patternFromString(testing::invert_pattern_string(freestyle_winning_patterns[i])));
		}

		testing::PatternGenerator generator(GameRules::FREESTYLE);
		matrix<Sign> board(15, 15);
		for (int i = 0; i < generator.numberOfPatterns(); i++)
			for (int r = -generator.patternLength(); r < board.rows() + generator.patternLength(); r++)
				for (int c = -generator.patternLength(); c < board.cols() + generator.patternLength(); c++)
					for (int dir = 0; dir < 4; dir++)
					{
						board.clear();
						bool is_valid = generator.applyPattern(i, static_cast<testing::Direction>(dir), board, r, c);
						if (is_valid)
						{
							GameOutcome winner = getOutcome(generator.rules, board);
							if (winner == GameOutcome::UNKNOWN)
							{
								EXPECT_FALSE(testing::is_winning(i, cross_winning_patterns));
								EXPECT_FALSE(testing::is_winning(i, circle_winning_patterns));
							}
							if (winner == GameOutcome::CROSS_WIN)
							{
								EXPECT_TRUE(testing::is_winning(i, cross_winning_patterns));
							}
							if (winner == GameOutcome::CIRCLE_WIN)
							{
								EXPECT_TRUE(testing::is_winning(i, circle_winning_patterns));
							}
						}
					}
	}
	TEST(TestRules, freestyleLastMove)
	{
		GTEST_SKIP();
		std::vector<int> cross_winning_patterns, circle_winning_patterns;
		for (size_t i = 0; i < freestyle_winning_patterns.size(); i++)
		{
			cross_winning_patterns.push_back(testing::patternFromString(freestyle_winning_patterns[i]));
			circle_winning_patterns.push_back(testing::patternFromString(testing::invert_pattern_string(freestyle_winning_patterns[i])));
		}

		testing::PatternGenerator generator(GameRules::FREESTYLE);
		matrix<Sign> board(15, 15);
		for (int i = 0; i < generator.numberOfPatterns(); i++)
			for (int r = -generator.patternLength(); r < board.rows() + generator.patternLength(); r++)
				for (int c = -generator.patternLength(); c < board.cols() + generator.patternLength(); c++)
					for (int dir = 0; dir < 4; dir++)
					{
						board.clear();
						bool is_valid = generator.applyPattern(i, static_cast<testing::Direction>(dir), board, r, c);
						if (is_valid)
							for (int x = r; x < r + generator.patternLength(); x++)
								for (int y = c; y < c + generator.patternLength(); y++)
									if (x >= 0 && x < board.rows() && y >= 0 && y < board.cols())
									{
										if (board.at(x, y) == Sign::CROSS)
										{
											GameOutcome winner = getOutcome(generator.rules, board, Move(x, y, board.at(x, y)));
											GameOutcome winner_full = getOutcome(generator.rules, board);
											if (winner_full == GameOutcome::CROSS_WIN)
											{
												EXPECT_EQ(winner, winner_full);
											}
										}
										if (board.at(x, y) == Sign::CIRCLE)
										{
											GameOutcome winner = getOutcome(generator.rules, board, Move(x, y, board.at(x, y)));
											GameOutcome winner_full = getOutcome(generator.rules, board);
											if (winner_full == GameOutcome::CIRCLE_WIN)
											{
												EXPECT_EQ(winner, winner_full);
											}
										}
									}
					}
	}
}

