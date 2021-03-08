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
	std::string signToString(Sign sign)
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
	std::ostream& operator<<(std::ostream &stream, Sign sign)
	{
		stream << signToString(sign);
		return stream;
	}
	std::string operator+(const std::string &lhs, Sign rhs)
	{
		return lhs + signToString(rhs);
	}
	std::string operator+(Sign lhs, const std::string &rhs)
	{
		return signToString(lhs) + rhs;
	}

	Move::Move(const Json &json) :
			row(static_cast<int>(json["row"])),
			col(static_cast<int>(json["col"])),
			sign(signFromString(json["sign"].getString()))
	{
	}
	std::string Move::toString() const
	{
		std::string result = signToString(sign) + " (";
		if (row < 10)
			result += ' ';
		result += std::to_string((int) row) + ',';
		if (row < 10)
			result += ' ';
		result += std::to_string((int) col) + ')';
		return result;
	}
	Json Move::serialize() const
	{
		return Json( { { "row", row }, { "col", col }, { "sign", signToString(sign) } });
	}
}

