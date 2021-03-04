/*
 * Move.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Move.hpp>

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
	std::ostream& operator<<(std::ostream &stream, Sign sign)
	{
		stream << toString(sign);
		return stream;
	}
	std::string operator+(const std::string &lhs, Sign rhs)
	{
		return lhs + toString(rhs);
	}
	std::string operator+(Sign lhs, const std::string &rhs)
	{
		return toString(lhs) + rhs;
	}

	std::string Move::toString() const
	{
		std::string result = ag::toString(sign) + " (";
		if (row < 10)
			result += ' ';
		result += std::to_string((int) row) + ',';
		if (row < 10)
			result += ' ';
		result += std::to_string((int) col) + ')';
		return result;
	}
}

