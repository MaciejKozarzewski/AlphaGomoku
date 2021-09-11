/*
 * rules.cpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/rules.hpp>

#include <stdexcept>

namespace ag
{
	std::string toString(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return "FREESTYLE";
			case GameRules::STANDARD:
				return "STANDARD";
			case GameRules::RENJU:
				return "RENJU";
			case GameRules::CARO:
				return "CARO";
			default:
				throw std::logic_error("unknown rule");
		}
	}
	GameRules rulesFromString(const std::string &str)
	{
		if (str == "FREESTYLE")
			return GameRules::FREESTYLE;
		if (str == "STANDARD")
			return GameRules::STANDARD;
		if (str == "RENJU")
			return GameRules::RENJU;
		if (str == "CARO")
			return GameRules::CARO;
		throw std::logic_error("unknown rule '" + str + "'");
	}

	std::string toString(GameOutcome outcome)
	{
		switch (outcome)
		{
			default:
			case GameOutcome::UNKNOWN:
				return "UNKNOWN";
			case GameOutcome::DRAW:
				return "DRAW";
			case GameOutcome::CROSS_WIN:
				return "CROSS_WIN";
			case GameOutcome::CIRCLE_WIN:
				return "CIRCLE_WIN";
		}
	}
	GameOutcome outcomeFromString(const std::string &str)
	{
		if (str == "UNKNOWN")
			return GameOutcome::UNKNOWN;
		if (str == "DRAW")
			return GameOutcome::DRAW;
		if (str == "CROSS_WIN")
			return GameOutcome::CROSS_WIN;
		if (str == "CIRCLE_WIN")
			return GameOutcome::CIRCLE_WIN;
		throw std::logic_error("unknown outcome '" + str + "'");
	}
} /* namespace ag */

