/*
 * Move.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>

#include <stdexcept>
#include <cassert>

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
	std::string toString(Sign sign)
	{
		switch (sign)
		{
			default:
			case Sign::NONE:
				return "NONE  ";
			case Sign::CROSS:
				return "CROSS ";
			case Sign::CIRCLE:
				return "CIRCLE";
		}
	}
	Sign signFromString(const std::string &str)
	{
		if (str == "CROSS")
			return Sign::CROSS;
		else
		{
			if (str == "CIRCLE")
				return Sign::CIRCLE;
			else
				return Sign::NONE;
		}
	}
	std::string text(Sign sign)
	{
		switch (sign)
		{
			case Sign::NONE:
				return "_";
			case Sign::CROSS:
				return "X";
			case Sign::CIRCLE:
				return "O";
			default:
			case Sign::ILLEGAL:
				return "|";
		}
	}
	std::ostream& operator<<(std::ostream &stream, Sign sign)
	{
		stream << ag::toString(sign);
		return stream;
	}
	std::string operator+(const std::string &lhs, Sign rhs)
	{
		return lhs + ag::toString(rhs);
	}
	std::string operator+(Sign lhs, const std::string &rhs)
	{
		return ag::toString(lhs) + rhs;
	}

	Location::Location(const std::string &str) :
			row(extract_row("_" + str)),
			col(extract_col("_" + str))
	{
	}
	std::string Location::toString() const
	{
		std::string result = "(";
		if (row < 10)
			result += ' ';
		result += std::to_string(row) + ',';
		if (col < 10)
			result += ' ';
		result += std::to_string(col) + ')';
		return result;
	}
	std::string Location::text() const
	{
		return static_cast<char>(static_cast<int>('a') + col) + std::to_string(row);
	}

	Move::Move(const std::string &str) :
			sign(extract_sign(str)),
			row(extract_row(str)),
			col(extract_col(str))
	{
	}
	std::string Move::toString() const
	{
		return ag::toString(sign) + " " + location().toString();
	}
	std::string Move::text() const
	{
		return ag::text(sign) + location().text();
	}
	Move Move::fromText(const std::string &txt, Sign sign)
	{
		assert(txt[0] >= 'a' && txt[0] <= 'z');
		return Move(ag::text(sign) + txt);
	}
}

