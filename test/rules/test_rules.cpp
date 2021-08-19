/*
 * test_rules.cpp
 *
 *  Created on: Mar 4, 2021
 *      Author: Maciej Kozarzewski
 */

#include "test_rules.hpp"

#include <alphagomoku/game/Move.hpp>
#include <stddef.h>
#include <array>
#include <iostream>
#include <vector>

namespace
{
	int pow(int base, int exponent) noexcept
	{
		int result = 1;
		for (int i = 0; i < exponent; i++)
			result *= base;
		return result;
	}
	int get_pattern_length(ag::GameRules rules) noexcept
	{
		switch (rules)
		{
			case ag::GameRules::FREESTYLE:
				return 6;
			case ag::GameRules::STANDARD:
				return 6;
			case ag::GameRules::RENJU:
				return 0; // TODO
			case ag::GameRules::CARO:
				return 7;
			default:
				return 0;
		}
	}
	ag::Sign char_to_sign(char c) noexcept
	{
		switch (c)
		{
			default:
			case '_':
				return ag::Sign::NONE;
			case 'X':
				return ag::Sign::CROSS;
			case 'O':
				return ag::Sign::CIRCLE;
		}
	}
}

namespace ag
{
	namespace testing
	{
		int patternFromString(const std::string &str) noexcept
		{
			int result = 0;
			int tmp = 1;
			for (size_t i = 0; i < str.size(); i++, tmp *= 3)
				result += tmp * static_cast<int>(char_to_sign(str[i]));
			return result;
		}
		bool is_winning(int pattern_to_check, const std::vector<int> &winning_patterns)
		{
			for (size_t j = 0; j < winning_patterns.size(); j++)
				if (pattern_to_check == winning_patterns[j])
					return true;
			return false;
		}
		std::string invert_pattern_string(const std::string &str)
		{
			std::string result;
			for (size_t i = 0; i < str.size(); i++)
				switch (str[i])
				{
					default:
						result += '_';
						break;
					case 'X':
						result += 'O';
						break;
					case 'O':
						result += 'X';
						break;
				}
			return result;
		}

		PatternGenerator::PatternGenerator(GameRules rules) :
				pattern_length(get_pattern_length(rules)),
				patterns(pow(3, pattern_length)),
				rules(rules)
		{
			generate_all_patterns();
		}
		std::string PatternGenerator::patternToString(int index) const noexcept
		{
			std::string result;
			for (int i = 0; i < pattern_length; i++)
				switch (patterns[index][i])
				{
					default:
					case Sign::NONE:
						result += '_';
						break;
					case Sign::CROSS:
						result += 'X';
						break;
					case Sign::CIRCLE:
						result += 'O';
						break;
				}
			return result;
		}
		int PatternGenerator::patternLength() const noexcept
		{
			return pattern_length;
		}
		int PatternGenerator::numberOfPatterns() const noexcept
		{
			return static_cast<int>(patterns.size());
		}
		bool PatternGenerator::applyPattern(int index, Direction dir, matrix<Sign> &board, int row, int col) const noexcept
		{
			switch (dir)
			{
				default:
					return false;
				case Direction::HORIZONTAL:
					return apply_pattern_horizontal(patterns[index], board, row, col);
				case Direction::VERTICAL:
					return apply_pattern_vertical(patterns[index], board, row, col);
				case Direction::DIAGONAL:
					return apply_pattern_diagonal(patterns[index], board, row, col);
				case Direction::ANTIDIAGONAL:
					return apply_pattern_antidiagonal(patterns[index], board, row, col);
			}
		}

		void PatternGenerator::generate_all_patterns() noexcept
		{
			for (size_t i = 0; i < patterns.size(); i++)
			{
				int tmp = static_cast<int>(i);
				for (int j = 0; j < pattern_length; j++)
				{
					patterns.at(i).at(j) = static_cast<Sign>(tmp % 3);
					tmp /= 3;
				}
			}
		}
		bool PatternGenerator::apply_pattern_horizontal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept
		{
			for (int i = 0; i < pattern_length; i++)
			{
				if (row < 0 || (col + i) < 0 || row >= board.rows() || (col + i) >= board.cols())
				{
					if (pattern[i] != Sign::NONE)
						return false;
				}
				else
					board.at(row, col + i) = pattern[i];
			}
			return true;
		}
		bool PatternGenerator::apply_pattern_vertical(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept
		{
			for (int i = 0; i < pattern_length; i++)
			{
				if ((row + i) < 0 || col < 0 || (row + i) >= board.rows() || col >= board.cols())
				{
					if (pattern[i] != Sign::NONE)
						return false;
				}
				else
					board.at(row + i, col) = pattern[i];
			}
			return true;
		}
		bool PatternGenerator::apply_pattern_diagonal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept
		{
			for (int i = 0; i < pattern_length; i++)
			{
				if ((row + i) < 0 || (col + i) < 0 || (row + i) >= board.rows() || (col + i) >= board.cols())
				{
					if (pattern[i] != Sign::NONE)
						return false;
				}
				else
					board.at(row + i, col + i) = pattern[i];
			}
			return true;
		}
		bool PatternGenerator::apply_pattern_antidiagonal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept
		{
			for (int i = 0; i < pattern_length; i++)
			{
				if ((row + pattern_length - 1 - i) < 0 || (col + i) < 0 || (row + pattern_length - 1 - i) >= board.rows()
						|| (col + i) >= board.cols())
				{
					if (pattern[i] != Sign::NONE)
						return false;
				}
				else
					board.at(row + pattern_length - 1 - i, col + i) = pattern[i];
			}
			return true;
		}
	}
}

