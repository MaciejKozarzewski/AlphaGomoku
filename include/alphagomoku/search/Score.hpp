/*
 * Score.hpp
 *
 *  Created on: Jan 26, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_SCORE_HPP_
#define ALPHAGOMOKU_SEARCH_SCORE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/Value.hpp>

#include <string>
#include <algorithm>
#include <cassert>

namespace ag
{
	enum class GameOutcome;
} /* namespace ag */

namespace ag
{

	enum class ProvenValue
	{
		LOSS, /* the position was proven to be a loss */
		DRAW, /* the position was proven to be a draw */
		UNKNOWN, /* the position has no proof yet */
		WIN /* the position was proven to be a win */
	};

	std::string toString(ProvenValue pv);
	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove);

	class Score
	{
			uint16_t m_data; // 3 bits for proven score, 13 bits for evaluation shifted by +4000
			constexpr explicit Score(uint16_t raw) noexcept :
					m_data(raw)
			{
			}
		public:
			constexpr Score() noexcept :
					Score(ProvenValue::UNKNOWN, 0)
			{
			}
			constexpr Score(ProvenValue pv) noexcept :
					Score(pv, 0)
			{
			}
			constexpr Score(int evaluation) noexcept :
					Score(ProvenValue::UNKNOWN, evaluation)
			{
			}
			constexpr Score(ProvenValue pv, int evaluation) noexcept :
					m_data(static_cast<uint16_t>(pv) << 13u)
			{
				assert(-4000 <= evaluation && evaluation <= 4000);
				m_data |= static_cast<uint16_t>(4000 + evaluation);
			}
			void increaseDistance() noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
					case ProvenValue::DRAW:
						*this = *this + 1;
						break;
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						*this = *this - 1;
						break;
				}
			}
			void decreaseDistance() noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
					case ProvenValue::DRAW:
						*this = *this - 1;
						break;
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						*this = *this + 1;
						break;
				}
			}
			int getDistance() const noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
					case ProvenValue::DRAW:
						return getEval();
					default:
					case ProvenValue::UNKNOWN:
						return 0;
					case ProvenValue::WIN:
						return -getEval();
				}
			}
			int getEval() const noexcept
			{
				return (m_data & 8191) - 4000;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return static_cast<ProvenValue>(m_data >> 13u);
			}
			bool isUnproven() const noexcept
			{
				return getProvenValue() == ProvenValue::UNKNOWN;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
			}
			bool isLoss() const noexcept
			{
				return getProvenValue() == ProvenValue::LOSS;
			}
			bool isDraw() const noexcept
			{
				return getProvenValue() == ProvenValue::DRAW;
			}
			bool isWin() const noexcept
			{
				return getProvenValue() == ProvenValue::WIN;
			}

			static Score min_value() noexcept
			{
				return Score(ProvenValue::LOSS, -4000);
			}
			static Score max_value() noexcept
			{
				return Score(ProvenValue::WIN, +4000);
			}
			static Score min_eval() noexcept
			{
				return Score(-4000);
			}
			static Score max_eval() noexcept
			{
				return Score(+4000);
			}

			static Score loss() noexcept
			{
				return Score(ProvenValue::LOSS);
			}
			static Score loss_in(int plys) noexcept
			{
				return Score(ProvenValue::LOSS, +plys);
			}
			static Score forbidden() noexcept
			{
				return Score(ProvenValue::LOSS);
			}
			static Score draw() noexcept
			{
				return Score(ProvenValue::DRAW);
			}
			static Score draw_in(int plys) noexcept
			{
				return Score(ProvenValue::DRAW, plys);
			}
			static Score win() noexcept
			{
				return Score(ProvenValue::WIN);
			}
			static Score win_in(int plys) noexcept
			{
				return Score(ProvenValue::WIN, -plys);
			}

			static Score from_short(uint16_t raw) noexcept
			{
				return Score(raw);
			}
			static uint16_t to_short(const Score &score) noexcept
			{
				return score.m_data;
			}

			friend Score operator+(const Score &a) noexcept
			{
				return a;
			}
			friend Score operator-(const Score &a) noexcept
			{
				switch (a.getProvenValue())
				{
					case ProvenValue::LOSS:
						return Score(ProvenValue::WIN, -a.getEval());
					case ProvenValue::DRAW:
						return Score(ProvenValue::DRAW, a.getEval());
					default:
					case ProvenValue::UNKNOWN:
						return Score(ProvenValue::UNKNOWN, -a.getEval());
					case ProvenValue::WIN:
						return Score(ProvenValue::LOSS, -a.getEval());
				}
			}
			friend Score operator+(const Score &a, int i) noexcept
			{
				return Score(a.getProvenValue(), a.getEval() + i);
			}
			friend Score operator-(const Score &a, int i) noexcept
			{
				return Score(a.getProvenValue(), a.getEval() - i);
			}

			friend bool operator==(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data == rhs.m_data;
			}
			friend bool operator!=(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data != rhs.m_data;
			}
			friend bool operator<(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data < rhs.m_data;
			}
			friend bool operator<=(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data <= rhs.m_data;
			}
			friend bool operator>(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data > rhs.m_data;
			}
			friend bool operator>=(const Score &lhs, const Score &rhs) noexcept
			{
				return lhs.m_data >= rhs.m_data;
			}
			Value convertToValue() const noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
						return Value::loss();
					case ProvenValue::DRAW:
						return Value::draw();
					case ProvenValue::UNKNOWN:
						return Value((1000 + getEval()) / 2000.0f, 0.0f);
					case ProvenValue::WIN:
						return Value::win();
					default:
						return Value();
				}
			}

			std::string toString() const;
			std::string toFormattedString() const;
	};

	inline std::ostream& operator<<(std::ostream &stream, Score score)
	{
		return stream << score.toString();
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_SCORE_HPP_ */
