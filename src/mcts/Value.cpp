/*
 * Value.cpp
 *
 *  Created on: Mar 26, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/rules/game_rules.hpp>

#include <assert.h>

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
	ProvenValue invert(ProvenValue pv) noexcept
	{
		switch (pv)
		{
			default:
			case ProvenValue::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case ProvenValue::LOSS:
				return ProvenValue::WIN;
			case ProvenValue::DRAW:
				return ProvenValue::DRAW;
			case ProvenValue::WIN:
				return ProvenValue::LOSS;
		}
	}

	Value convertOutcome(GameOutcome outcome, Sign signToMove)
	{
		assert(outcome != GameOutcome::UNKNOWN);
		switch (outcome)
		{
			default:
			case GameOutcome::DRAW:
				return Value(0.0f, 1.0f, 0.0f);
			case GameOutcome::CROSS_WIN:
				return (signToMove == Sign::CIRCLE) ? Value(1.0f, 0.0f, 0.0f) : Value(0.0f, 0.0f, 1.0f);
			case GameOutcome::CIRCLE_WIN:
				return (signToMove == Sign::CROSS) ? Value(1.0f, 0.0f, 0.0f) : Value(0.0f, 0.0f, 1.0f);
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
				return (signToMove == Sign::CIRCLE) ? ProvenValue::WIN : ProvenValue::LOSS;
			case GameOutcome::CIRCLE_WIN:
				return (signToMove == Sign::CROSS) ? ProvenValue::WIN : ProvenValue::LOSS;
		}
	}
}

