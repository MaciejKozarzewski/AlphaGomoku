/*
 * Move.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Move.hpp>
#include <libml/utils/json.hpp>

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

	Move::Move(const Json &json) :
			row(static_cast<int>(json["row"])),
			col(static_cast<int>(json["col"])),
			sign(signFromString(json["sign"].getString()))
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
	Json Move::serialize() const
	{
		return Json( { { "row", row }, { "col", col }, { "sign", ag::toString(sign) } });
	}
}

