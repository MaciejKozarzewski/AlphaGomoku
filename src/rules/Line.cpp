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
#include <cassert>

namespace
{
	uint64_t pattern_from_string(const std::string &text)
	{
		uint64_t result = 0;
		for (size_t i = 0; i < std::min(static_cast<size_t>(16), text.size()); i++)
			switch (text[text.size() - 1 - i])
			{
				// using one-hot encoding
				// sub-patterns here are inverted for a purpose, the whole patters has to have ones outside
				// the area of interest which is accomplished by negating the result at the end
				case '_':
					result = (result << 4) | 1; // 0001
					break;
				case 'X':
					result = (result << 4) | 2; // 0010
					break;
				case 'O':
					result = (result << 4) | 4; // 0100
					break;
				case '|':
					result = (result << 4) | 8; // 1000
					break;
				case 'a': // indicates that any sign can be in this place
					result = (result << 4) | 15; //1111
					break;
			}
		return result;
	}
}

namespace ag
{

	Line::Line(GameRules rules) :
			rules(rules)
	{
	}

	void Line::fill(const std::string &text)
	{
		line = pattern_from_string(text);
	}

	bool Line::is_cross_five() noexcept
	{
		if (cross_five.was_evaluated)
			return cross_five.value;
		return false;
	}
	bool Line::is_cross_open_four() noexcept
	{
		if (cross_open_four.was_evaluated)
			return cross_open_four.value;
		return false;
	}
	bool Line::is_cross_four() noexcept
	{
		if (cross_four.was_evaluated)
			return cross_four.value;
		return false;
	}
	bool Line::is_cross_open_three() noexcept
	{
		if (cross_open_three.was_evaluated)
			return cross_open_three.value;
		return false;
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
				case 4:
					result[i] = '|';
					break;
				case 15:
					result[i] = 'a';
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

