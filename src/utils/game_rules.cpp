/*
 * game_rules.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/rules/freestyle.hpp>
#include <alphagomoku/rules/standard.hpp>
#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/caro.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <stdexcept>

namespace ag
{

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
		throw std::logic_error("unknown rule");
	}
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

	GameOutcome outcomeFromString(const std::string &str)
	{
		if (str == "DRAW")
			return GameOutcome::DRAW;
		if (str == "CROSS_WIN")
			return GameOutcome::CROSS_WIN;
		if (str == "CIRCLE_WIN")
			return GameOutcome::CIRCLE_WIN;
		return GameOutcome::UNKNOWN;
	}
	std::string outcomeToString(GameOutcome outcome)
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

	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return getOutcomeFreestyle(board);
			case GameRules::STANDARD:
				return getOutcomeStandard(board);
			case GameRules::RENJU:
				return getOutcomeRenju(board);
			case GameRules::CARO:
				return getOutcomeCaro(board);
		}
	}
	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board, const Move &last_move)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return getOutcomeFreestyle(board, last_move);
			case GameRules::STANDARD:
				return getOutcomeStandard(board, last_move);
			case GameRules::RENJU:
				return getOutcomeRenju(board, last_move);
			case GameRules::CARO:
				return getOutcomeCaro(board, last_move);
		}
	}
}

