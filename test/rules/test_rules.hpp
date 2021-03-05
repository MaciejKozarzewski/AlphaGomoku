/*
 * test_rules.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#ifndef RULES_TEST_RULES_HPP_
#define RULES_TEST_RULES_HPP_

#include <alphagomoku/utils/game_rules.hpp>

namespace ag
{
	namespace testing
	{
		enum class Direction
		{
			HORIZONTAL, VERTICAL, DIAGONAL, ANTIDIAGONAL
		};

		int patternFromString(const std::string &str) noexcept;
		bool is_winning(int pattern_to_check, const std::vector<int> &winning_patterns);
		std::string invert_pattern_string(const std::string &str);

		class PatternGenerator
		{
				int pattern_length;
				std::vector<std::array<Sign, 10>> patterns;

			public:
				const GameRules rules;
				PatternGenerator(GameRules rules);
				std::string patternToString(int index) const noexcept;
				int patternLength() const noexcept;
				int numberOfPatterns() const noexcept;
				void applyPattern(int index, Direction dir, matrix<Sign> &board, int row, int col) const noexcept;
			private:
				void generate_all_patterns() noexcept;

				void apply_pattern_horizontal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept;
				void apply_pattern_vertical(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept;
				void apply_pattern_diagonal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept;
				void apply_pattern_antidiagonal(const std::array<Sign, 10> &pattern, matrix<Sign> &board, int row, int col) const noexcept;
		};
	}
}

#endif /* RULES_TEST_RULES_HPP_ */
