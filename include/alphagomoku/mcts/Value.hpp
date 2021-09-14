/*
 * Value.hpp
 *
 *  Created on: Mar 26, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_VALUE_HPP_
#define ALPHAGOMOKU_MCTS_VALUE_HPP_

#include <alphagomoku/game/Move.hpp>

#include <string>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace ag
{
	enum class GameOutcome
	;
} /* namespace ag */

namespace ag
{
	enum class ProvenValue : int16_t
	{
		UNKNOWN,
		LOSS,
		DRAW,
		WIN
	};

	std::string toString(ProvenValue pv);
	ProvenValue invert(ProvenValue pv) noexcept;

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
			friend bool operator ==(const Value &lhs, const Value &rhs) noexcept
			{
				return lhs.win == rhs.win and lhs.draw == rhs.draw and lhs.loss == rhs.loss;
			}
			friend bool operator !=(const Value &lhs, const Value &rhs) noexcept
			{
				return lhs.win != rhs.win or lhs.draw != rhs.draw or lhs.loss != rhs.loss;
			}
			friend Value operator+(const Value &lhs, const Value &rhs) noexcept
			{
				return Value(lhs.win + rhs.win, lhs.draw + rhs.draw, lhs.loss + rhs.loss);
			}
			friend Value operator-(const Value &lhs, const Value &rhs) noexcept
			{
				return Value(lhs.win - rhs.win, lhs.draw - rhs.draw, lhs.loss - rhs.loss);
			}
			friend Value operator*(const Value &lhs, float rhs) noexcept
			{
				return Value(lhs.win * rhs, lhs.draw * rhs, lhs.loss * rhs);
			}
	};

	Value convertOutcome(GameOutcome outcome, Sign signToMove);
	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove);
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_VALUE_HPP_ */
