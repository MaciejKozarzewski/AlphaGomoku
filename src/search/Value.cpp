/*
 * Value.cpp
 *
 *  Created on: Mar 26, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/game/rules.hpp>

#include <cassert>

namespace ag
{

	Value convertOutcome(GameOutcome outcome, Sign signToMove)
	{
		assert(outcome != GameOutcome::UNKNOWN);
		switch (outcome)
		{
			default:
			case GameOutcome::DRAW:
				return Value::draw();
			case GameOutcome::CROSS_WIN:
				return (signToMove == Sign::CROSS) ? Value::win() : Value::loss();
			case GameOutcome::CIRCLE_WIN:
				return (signToMove == Sign::CIRCLE) ? Value::win() : Value::loss();
		}
	}

} /* namespace ag */

