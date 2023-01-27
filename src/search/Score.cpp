/*
 * Score.cpp
 *
 *  Created on: Jan 26, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/rules.hpp>

namespace ag
{
	std::string toString(ProvenValue pv)
	{
		switch (pv)
		{
			default:
			case ProvenValue::UNKNOWN:
				return "UNKNOWN";
			case ProvenValue::LOSS:
				return "LOSS";
			case ProvenValue::DRAW:
				return "DRAW";
			case ProvenValue::WIN:
				return "WIN";
		}
	}

	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove)
	{
		switch (outcome)
		{
			default:
			case GameOutcome::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case GameOutcome::DRAW:
				return ProvenValue::DRAW;
			case GameOutcome::CROSS_WIN:
				return (signToMove == Sign::CROSS) ? ProvenValue::WIN : ProvenValue::LOSS;
			case GameOutcome::CIRCLE_WIN:
				return (signToMove == Sign::CIRCLE) ? ProvenValue::WIN : ProvenValue::LOSS;
		}
	}

} /* namespace ag */

