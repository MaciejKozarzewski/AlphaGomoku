/*
 * Score.hpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_SCORE_HPP_
#define ALPHAGOMOKU_TSS_SCORE_HPP_

#include <cinttypes>
#include <algorithm>
#include <string>
#include <cassert>

namespace ag
{
	/*
	 * All combinations
	 *
	 * must_defend == false	-> we try to proven a win
	 *   score starts from NOT_LOSS	(even if all moves are losing it's not a loss because but we didn't have to defend. Worst outcome we can have here is simply no proof)
	 *
	 *   Possible outcomes:
	 *        max_score == LOSS      ->  N/A
	 *        max_score == DRAW      ->  proven score = DRAW
	 *        max_score == UNKNOWN   ->  proven score = UNKNOWN		(no proved win and at least one move is not proven at all)
	 *        max_score == WIN       ->  proven score = WIN			(at least one move is winning, we can stop here)

	 *
	 * must_defend == true	-> we try to prove a loss
	 *   assert(num children() > 0)
	 *   score starts from LOSS
	 *   Possible outcomes:
	 *        max_score == LOSS      ->  proven score = LOSS		(all moves are losing)
	 *        max_score == DRAW      ->  proven score = DRAW
	 *        max_score == UNKNOWN   ->  proven score = UNKNOWN		(no proved win and at least one move is not proven at all)
	 *        max_score == WIN       ->  proven score = WIN			(at least one move is winning)

	 *
	 * ProvenScore inversion rule:
	 *  LOSS     -> WIN
	 *  DRAW     -> DRAW
	 *	UNKNOWN  -> UNKNOWN
	 *  WIN      -> LOSS
	 */
	enum class ProvenScore
	{
		LOSS, /* the position was proven to be a loss */
		DRAW, /* the position was proven to be a draw */
		UNKNOWN, /* the position has no proof yet */
		WIN /* the position was proven to be a win */
	};

	inline ProvenScore invertProvenScore(ProvenScore ps) noexcept
	{
		switch (ps)
		{
			case ProvenScore::LOSS:
				return ProvenScore::WIN;
			case ProvenScore::DRAW:
				return ProvenScore::DRAW;
			default:
			case ProvenScore::UNKNOWN:
				return ProvenScore::UNKNOWN;
			case ProvenScore::WIN:
				return ProvenScore::LOSS;
		}
	}

	class Score
	{
			uint16_t m_data; // 3 bits for proven score, 13 bits for evaluation shifted by +4000
			Score(uint16_t raw) :
					m_data(raw)
			{
			}
		public:
			Score() noexcept :
					Score(ProvenScore::UNKNOWN, 0)
			{
			}
			Score(ProvenScore ps) :
					Score(ps, 0)
			{
			}
			Score(int evaluation) :
					Score(ProvenScore::UNKNOWN, evaluation)
			{
			}
			Score(ProvenScore ps, int evaluation) :
					m_data(static_cast<uint16_t>(ps) << 13)
			{
				assert(-4000 <= evaluation && evaluation <= 4000);
				m_data |= static_cast<uint16_t>(4000 + evaluation);
			}
			void increaseDistanceToWinOrLoss() noexcept
			{
				switch (getProvenScore())
				{
					default:
						break;
					case ProvenScore::LOSS:
						*this = *this + 1;
						break;
					case ProvenScore::WIN:
						*this = *this - 1;
						break;
				}
			}
			int getEval() const noexcept
			{
				return (m_data & 8191) - 4000;
			}
			ProvenScore getProvenScore() const noexcept
			{
				return static_cast<ProvenScore>(m_data >> 13);
			}
			bool isProven() const noexcept
			{
				return getProvenScore() != ProvenScore::UNKNOWN;
			}
			bool isLoss() const noexcept
			{
				return getProvenScore() == ProvenScore::LOSS;
			}
			bool isDraw() const noexcept
			{
				return getProvenScore() == ProvenScore::DRAW;
			}
			bool isWin() const noexcept
			{
				return getProvenScore() == ProvenScore::WIN;
			}

			static Score min_value() noexcept
			{
				return Score(ProvenScore::LOSS, -4000);
			}
			static Score max_value() noexcept
			{
				return Score(ProvenScore::WIN, +4000);
			}

			static Score loss() noexcept
			{
				return Score(ProvenScore::LOSS);
			}
			static Score loss_in(int plys) noexcept
			{
				return Score(ProvenScore::LOSS, +plys);
			}
			static Score forbidden() noexcept
			{
				return Score(ProvenScore::LOSS);
			}
			static Score draw() noexcept
			{
				return Score(ProvenScore::DRAW);
			}
			static Score win() noexcept
			{
				return Score(ProvenScore::WIN);
			}
			static Score win_in(int plys) noexcept
			{
				return Score(ProvenScore::WIN, -plys);
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
				return Score(invertProvenScore(a.getProvenScore()), -a.getEval());
			}
			friend Score operator+(const Score &a, int i) noexcept
			{
				return Score(a.getProvenScore(), a.getEval() + i);
			}
			friend Score operator-(const Score &a, int i) noexcept
			{
				return Score(a.getProvenScore(), a.getEval() - i);
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
				switch (getProvenScore())
				{
					case ProvenScore::LOSS:
						return "LOSS in " + std::to_string(getEval());
					case ProvenScore::DRAW:
						return "DRAW";
					default:
					case ProvenScore::UNKNOWN:
						if (getEval() > 0)
							return "+" + std::to_string(getEval());
						else
							return std::to_string(getEval());
					case ProvenScore::WIN:
						return "WIN in " + std::to_string(-getEval());
				}
			}
	};

	inline std::ostream& operator<<(std::ostream &stream, Score score)
	{
		return stream << score.toString();
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_SCORE_HPP_ */
