/*
 * Move.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>
#include <libml/utils/json.hpp>

namespace
{
	using namespace ag;
	Sign extract_sign(const std::string &text)
	{
		assert(text.length() >= 1);
		switch (text[0])
		{
			case '_':
				return Sign::NONE;
			case 'X':
				return Sign::CROSS;
			case 'O':
				return Sign::CIRCLE;
			default:
				throw std::invalid_argument("expected 'X' or 'O'");
		}
	}
	int extract_row(const std::string &text)
	{
		assert(text.length() >= 3);
		return std::stoi(text.substr(2, text.length()));
	}
	int extract_col(const std::string &text)
	{
		assert(text.length() >= 2);
		return static_cast<int>(text[1]) - static_cast<int>('a');
	}
}
namespace ag
{

	Move::Move(const std::string &str) :
			row(extract_row(str)),
			col(extract_col(str)),
			sign(extract_sign(str))
	{
	}
	std::string Move::toString() const
	{
		std::string result = ag::toString(sign) + " (";
		if (row < 10)
			result += ' ';
		result += std::to_string(row) + ',';
		if (col < 10)
			result += ' ';
		result += std::to_string(col) + ')';
		return result;
	}
	std::string Move::text() const
	{
		return ag::text(sign) + static_cast<char>(static_cast<int>('a') + col) + std::to_string(row);
	}
}

