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

	Move Move::fromText(const std::string &text)
	{
		return Move(extract_row(text), extract_col(text), extract_sign(text));
	}
}

