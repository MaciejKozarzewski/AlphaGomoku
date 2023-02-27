/*
 * Value.hpp
 *
 *  Created on: Mar 26, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_VALUE_HPP_
#define ALPHAGOMOKU_SEARCH_VALUE_HPP_

#include <alphagomoku/game/Move.hpp>

#include <string>
#include <cmath>
#include <algorithm>
#include <cassert>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{

	struct Value
	{
			float win_rate = 0.0f;
			float draw_rate = 0.0f;

			Value() noexcept = default;
			Value(float w, float d = 0.0f) noexcept :
					win_rate(w),
					draw_rate(d)
			{
			}
			float loss_rate() const noexcept
			{
				return 1.0f - (win_rate + draw_rate);
			}
			float getExpectation(float w = 0.5f) const noexcept
			{
				assert(0.0f <= w && w <= 1.0f);
				return win_rate + w * draw_rate;
			}
			Value getInverted() const noexcept
			{
				return Value(loss_rate(), draw_rate);
			}
			void invert() noexcept
			{
				win_rate = loss_rate();
			}
			std::string toString() const
			{
				return std::to_string(win_rate) + " : " + std::to_string(draw_rate) + " : " + std::to_string(loss_rate());
			}
			friend bool operator ==(const Value &lhs, const Value &rhs) noexcept
			{
				return lhs.win_rate == rhs.win_rate and lhs.draw_rate == rhs.draw_rate;
			}
			friend bool operator !=(const Value &lhs, const Value &rhs) noexcept
			{
				return lhs.win_rate != rhs.win_rate or lhs.draw_rate != rhs.draw_rate;
			}
			friend Value operator+(const Value &lhs, const Value &rhs) noexcept
			{
				return Value(lhs.win_rate + rhs.win_rate, lhs.draw_rate + rhs.draw_rate);
			}
			friend Value& operator+=(Value &lhs, const Value &rhs) noexcept
			{
				lhs = lhs + rhs;
				return lhs;
			}
			friend Value operator-(const Value &lhs, const Value &rhs) noexcept
			{
				return Value(lhs.win_rate - rhs.win_rate, lhs.draw_rate - rhs.draw_rate);
			}
			friend Value operator*(const Value &lhs, float rhs) noexcept
			{
				return Value(lhs.win_rate * rhs, lhs.draw_rate * rhs);
			}
			friend Value& operator*=(Value &lhs, float rhs) noexcept
			{
				lhs = lhs * rhs;
				return lhs;
			}
			float abs() const noexcept
			{
				return fabsf(win_rate) + fabsf(draw_rate);
			}
			bool isValid() const noexcept
			{
				return (-0.001f <= win_rate and win_rate <= 1.001f) and (-0.001f <= draw_rate and draw_rate <= 1.001f)
						and (-0.001f <= loss_rate() and loss_rate() <= 1.001f);
			}
			void clipToBounds() noexcept
			{
				win_rate = std::max(0.0f, std::min(1.0f, win_rate));
				draw_rate = std::max(0.0f, std::min(1.0f, draw_rate));
			}

			static Value win() noexcept
			{
				return Value(1.0f, 0.0f);
			}
			static Value draw() noexcept
			{
				return Value(0.0f, 1.0f);
			}
			static Value loss() noexcept
			{
				return Value(0.0f, 0.0f);
			}
	};

	Value convertOutcome(GameOutcome outcome, Sign signToMove);

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_VALUE_HPP_ */
