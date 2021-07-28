/*
 * Line.cpp
 *
 *  Created on: Jul 27, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/rules/Line.hpp>
#include <alphagomoku/rules/game_rules.hpp>

#include <bitset>
#include <algorithm>
#include <assert.h>

namespace
{
	uint32_t pattern_from_string(const std::string &text)
	{
		uint32_t result = 0;
		for (size_t i = 0; i < std::min(static_cast<size_t>(16), text.size()); i++)
			switch (text[text.size() - 1 - i])
			{
				// sub-patterns here are inverted for a purpose, the whole patters has to have ones outside
				// the area of interest which is accomplished by negating the result at the end
				case '_':
					result = (result << 2) | 3;
					break;
				case 'X':
					result = (result << 2) | 2;
					break;
				case 'O':
					result = (result << 2) | 1;
					break;
				case '?': // indicates that any sign can be in this place
				case '|':
					result = (result << 2) | 0;
					break;
			}
		return ~result;
	}
}

namespace ag
{

	Line::Line(const std::string &text, GameRules rules) :
			line(pattern_from_string(text)),
			rules(rules)
	{
	}

	bool Line::is_cross_five() noexcept
	{
		if (cross_five.was_evaluated)
			return cross_five.value;

	}
	bool Line::is_cross_open_four() noexcept
	{
		if (cross_open_four.was_evaluated)
			return cross_open_four.value;
	}
	bool Line::is_cross_four() noexcept
	{
		if (cross_four.was_evaluated)
			return cross_four.value;
	}
	bool Line::is_cross_open_three() noexcept
	{
		if (cross_open_three.was_evaluated)
			return cross_open_three.value;
	}

	std::string Line::toString() const
	{
		std::string result(11, '\0');
		uint32_t tmp = line;
		for (uint32_t i = 0; i < 11; i++)
		{
			switch (tmp & 3)
			{
				case 0:
					result[i] = '_';
					break;
				case 1:
					result[i] = 'X';
					break;
				case 2:
					result[i] = 'O';
					break;
				case 3:
					result[i] = '|';
					break;
			}
			tmp = tmp >> 2;
		}
		return result;
	}
	std::string Line::toBinaryString() const
	{
		return std::bitset<32>(line).to_string();
	}

} /* namespace ag */

