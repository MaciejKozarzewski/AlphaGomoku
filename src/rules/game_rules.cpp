/*
 * game_rules.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/rules/freestyle.hpp>
#include <alphagomoku/rules/standard.hpp>
#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/caro.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <stdexcept>

namespace ag
{

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

	GameOutcome getOutcome(GameRules rules, const std::vector<Sign> &line)
	{
		switch (rules)
		{
			default:
				throw std::logic_error("unknown rule");
			case GameRules::FREESTYLE:
				return getOutcomeFreestyle(line);
			case GameRules::STANDARD:
				return getOutcomeStandard(line);
			case GameRules::RENJU:
				return getOutcomeRenju(line);
			case GameRules::CARO:
				return getOutcomeCaro(line);
		}
	}
}

