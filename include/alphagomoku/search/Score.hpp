/*
 * Score.hpp
 *
 *  Created on: Jan 26, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SCORE_HPP_
#define ALPHAGOMOKU_MCTS_SCORE_HPP_

#include <alphagomoku/game/Move.hpp>

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

	/*
	 * ProvenValue inversion rules:
	 *  LOSS     -> WIN
	 *  DRAW     -> DRAW
	 *	UNKNOWN  -> UNKNOWN
	 *  WIN      -> LOSS
	 */
	inline ProvenValue invertProvenValue(ProvenValue pv) noexcept
	{
		switch (pv)
		{
			case ProvenValue::LOSS:
				return ProvenValue::WIN;
			case ProvenValue::DRAW:
				return ProvenValue::DRAW;
			default:
			case ProvenValue::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case ProvenValue::WIN:
				return ProvenValue::LOSS;
		}
	}

	class Score
	{
			uint16_t m_data; // 3 bits for proven score, 13 bits for evaluation shifted by +4000
			explicit Score(uint16_t raw) :
					m_data(raw)
			{
			}
		public:
			Score() noexcept :
					Score(ProvenValue::UNKNOWN, 0)
			{
			}
			Score(ProvenValue pv) :
					Score(pv, 0)
			{
			}
			Score(int evaluation) :
					Score(ProvenValue::UNKNOWN, evaluation)
			{
			}
			Score(ProvenValue pv, int evaluation) :
					m_data(static_cast<uint16_t>(pv) << 13u)
			{
				assert(-4000 <= evaluation && evaluation <= 4000);
				m_data |= static_cast<uint16_t>(4000 + evaluation);
			}
			void increaseDistanceToWinOrLoss() noexcept
			{
				switch (getProvenValue())
				{
					default:
						break;
					case ProvenValue::LOSS:
						*this = *this + 1;
						break;
					case ProvenValue::WIN:
						*this = *this - 1;
						break;
				}
			}
			int getDistanceToWinOrLoss() const noexcept
			{
				switch (getProvenValue())
				{
					default:
						return 4000;
					case ProvenValue::LOSS:
						return getEval();
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
				return Score(invertProvenValue(a.getProvenValue()), -a.getEval());
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
			std::string toString() const
			{
				switch (getProvenValue())
				{
					case ProvenValue::LOSS:
						return "LOSS in " + std::to_string(getEval());
					case ProvenValue::DRAW:
						return "DRAW";
					default:
					case ProvenValue::UNKNOWN:
						if (getEval() > 0)
							return "+" + std::to_string(getEval());
						else
							return std::to_string(getEval());
					case ProvenValue::WIN:
						return "WIN in " + std::to_string(-getEval());
				}
			}
	};

	inline std::ostream& operator<<(std::ostream &stream, Score score)
	{
		return stream << score.toString();
	}

} /* namespace */

#endif /* ALPHAGOMOKU_MCTS_SCORE_HPP_ */
