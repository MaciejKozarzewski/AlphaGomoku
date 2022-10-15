/*
 * DefensiveMoveTable.hpp
 *
 *  Created on: Oct 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_
#define ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/BitMask.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>

#include <algorithm>
#include <cassert>

namespace ag
{
	class DefensiveMoves
	{
			std::array<int8_t, 5> m_offsets;
			int8_t m_length = 0;
		public:
			DefensiveMoves() noexcept = default;
			DefensiveMoves(BitMask1D<uint16_t> mask, int center = 6) noexcept
			{
				for (int i = 0; i < mask.size(); i++)
					if (mask[i])
						add(i - center);
			}
			DefensiveMoves(std::initializer_list<int> list) :
					m_length(list.size())
			{
				for (int i = 0; i < m_length; i++)
					m_offsets[i] = list.begin()[i];
			}
			void add(int offset) noexcept
			{
				assert(m_length < 5);
				m_offsets[m_length++] = offset;
			}
			int length() const noexcept
			{
				return m_length;
			}
			int operator[](int index) const noexcept
			{
				assert(0 <= index && index < length());
				return m_offsets[index];
			}
			void sort() noexcept
			{
				std::sort(m_offsets.begin(), m_offsets.begin() + m_length);
			}
			void flip() noexcept
			{
				for (int i = 0; i < 5; i++)
					m_offsets[i] = -m_offsets[i];
			}
			void shift(int s) noexcept
			{
				for (int i = 0; i < 5; i++)
					m_offsets[i] += s;
			}
	};

	/*
	 * \brief Returns list of moves that can be used to promote an open three into a four (intended only for cross in renju rule).
	 */
	BitMask1D<uint16_t> getOpenThreePromotionMoves(uint32_t pattern) noexcept;

	class DefensiveMoveTable
	{
			template<typename T>
			struct SignPair
			{
					T for_cross = T { };
					T for_circle = T { };
					T get(Sign s) const noexcept
					{
						assert(s == Sign::CROSS || s == Sign::CIRCLE);
						return (s == Sign::CROSS) ? for_cross : for_circle;
					}
			};

			matrix<SignPair<BitMask1D<uint16_t>>> five_defense;
			matrix<SignPair<BitMask1D<uint16_t>>> open_four_defense;
			matrix<SignPair<BitMask1D<uint16_t>>> double_four_defense;
			GameRules game_rules;
		public:
			DefensiveMoveTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			BitMask1D<uint16_t> getMoves(uint32_t pattern, Sign defenderSign, PatternType threatToDefend) const;
			static const DefensiveMoveTable& get(GameRules rules);
		private:
			void init_five();
			void init_open_four();
			void init_double_four();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_ */
