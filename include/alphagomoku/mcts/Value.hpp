/*
 * Value.hpp
 *
 *  Created on: Mar 26, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_VALUE_HPP_
#define ALPHAGOMOKU_MCTS_VALUE_HPP_

#include <string>
#include <algorithm>
#include <assert.h>

namespace ag
{
	enum class GameOutcome;
	enum class Sign;
} /* namespace ag */

namespace ag
{
	enum class ProvenValue
	{
		UNKNOWN,
		LOSS,
		DRAW,
		WIN
	};

	std::string toString(ProvenValue pv);

	struct Value
	{
			float win = 0.0f;
			float draw = 0.0f;
			float loss = 0.0f;

			Value() = default;
			Value(float w) :
					win(w)
			{
			}
			Value(float w, float d, float l) noexcept :
					win(w),
					draw(d),
					loss(l)
			{
			}
			Value getInverted() const noexcept
			{
				return Value(loss, draw, win);
			}
			void invert() noexcept
			{
				std::swap(win, loss);
			}
			std::string toString() const
			{
				return std::to_string(win) + " : " + std::to_string(draw) + " : " + std::to_string(loss);
			}
			friend bool operator ==(const Value &lhs, const Value &rhs)
			{
				return lhs.win == rhs.win and lhs.draw == rhs.draw and lhs.loss == rhs.loss;
			}
			friend bool operator !=(const Value &lhs, const Value &rhs)
			{
				return lhs.win != rhs.win or lhs.draw != rhs.draw or lhs.loss != rhs.loss;
			}
	};

	Value convertOutcome(GameOutcome outcome, Sign signToMove);
	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove);
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_VALUE_HPP_ */
