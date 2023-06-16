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

	enum class Bound
	{
		NONE,
		LOWER,
		UPPER,
		EXACT
	};

	std::string toString(Bound b);

	class Score
	{
			uint16_t m_data; // 3 bits for proven score, 13 bits for evaluation shifted by +4000
		public:
			constexpr Score() noexcept :
					Score(ProvenValue::UNKNOWN, 0)
			{
			}
			constexpr explicit Score(ProvenValue pv) noexcept :
					Score(pv, 0)
			{
			}
			constexpr explicit Score(int evaluation) noexcept :
					Score(ProvenValue::UNKNOWN, evaluation)
			{
			}
			constexpr explicit Score(ProvenValue pv, int evaluation) noexcept :
					m_data(static_cast<uint16_t>(pv) << 13u)
			{
				assert(-4000 <= evaluation && evaluation <= 4000);
				m_data |= static_cast<uint16_t>(4000 + evaluation);
			}
			void increaseDistance(int i = 1) noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
					case ProvenValue::DRAW:
						*this = *this + i;
						break;
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						*this = *this - i;
						break;
				}
			}
			void decreaseDistance(int i = 1) noexcept
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
					case ProvenValue::DRAW:
						*this = *this - i;
						break;
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::WIN:
						*this = *this + i;
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
				return static_cast<ProvenValue>((m_data >> 13u) & 3u);
			}
			bool isUnproven() const noexcept
			{
				return getProvenValue() == ProvenValue::UNKNOWN;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN and isFinite();
			}
			bool isLoss() const noexcept
			{
				return getProvenValue() == ProvenValue::LOSS and isFinite();
			}
			bool isDraw() const noexcept
			{
				return getProvenValue() == ProvenValue::DRAW;
			}
			bool isWin() const noexcept
			{
				return getProvenValue() == ProvenValue::WIN and isFinite();
			}

			bool isFinite() const noexcept
			{
				return not isInfinite();
			}
			bool isInfinite() const noexcept
			{
				return *this == Score::minus_infinity() or *this == Score::plus_infinity();
			}
			static Score minus_infinity() noexcept
			{
				return Score::from_short(static_cast<uint16_t>(0x0000));
			}
			static Score plus_infinity() noexcept
			{
				return Score::from_short(static_cast<uint16_t>(0xFFFF));
			}

			static Score min_value() noexcept
			{
				return minus_infinity();
			}
			static Score max_value() noexcept
			{
				return plus_infinity();
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

			static constexpr Score from_short(uint16_t raw) noexcept
			{
				Score result;
				result.m_data = raw;
				return result;
			}
			static constexpr uint16_t to_short(const Score &score) noexcept
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
						return a.isFinite() ? Score(ProvenValue::WIN, -a.getEval()) : Score::plus_infinity();
					case ProvenValue::DRAW:
						return Score(ProvenValue::DRAW, a.getEval());
					default:
					case ProvenValue::UNKNOWN:
						return Score(ProvenValue::UNKNOWN, -a.getEval());
					case ProvenValue::WIN:
						return a.isFinite() ? Score(ProvenValue::LOSS, -a.getEval()) : Score::minus_infinity();
				}
			}
			friend Score operator+(const Score &a, int i) noexcept
			{
				return a.isFinite() ? Score(a.getProvenValue(), a.getEval() + i) : a;
			}
			friend Score operator-(const Score &a, int i) noexcept
			{
				return a.isFinite() ? Score(a.getProvenValue(), a.getEval() - i) : a;
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
						return isFinite() ? Value::loss() : Value();
					case ProvenValue::DRAW:
						return Value::draw();
					case ProvenValue::UNKNOWN:
						return Value((1000 + getEval()) / 2000.0f, 0.0f);
					case ProvenValue::WIN:
						return isFinite() ? Value::win() : Value();
					default:
						return Value();
				}
			}
			/*
			 * Invert score and increase the distance to win/loss
			 */
			friend Score invert_up(Score s) noexcept
			{
				switch (s.getProvenValue())
				{
					case ProvenValue::LOSS:
						return s.isFinite() ? Score::win_in(s.getDistance() + 1) : -s;
					case ProvenValue::DRAW:
						return Score::draw_in(s.getDistance() + 1);
					default:
					case ProvenValue::UNKNOWN:
						return -s;
					case ProvenValue::WIN:
						return s.isFinite() ? Score::loss_in(s.getDistance() + 1) : -s;
				}
			}
			/*
			 * Invert score and decrease the distance to win/loss
			 */
			friend Score invert_down(Score s) noexcept
			{
				switch (s.getProvenValue())
				{
					case ProvenValue::LOSS:
						return s.isFinite() ? Score::win_in(s.getDistance() - 1) : -s;
					case ProvenValue::DRAW:
						return Score::draw_in(s.getDistance() - 1);
					default:
					case ProvenValue::UNKNOWN:
						return -s;
					case ProvenValue::WIN:
						return s.isFinite() ? Score::loss_in(s.getDistance() - 1) : -s;
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
