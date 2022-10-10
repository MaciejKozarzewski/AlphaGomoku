/*
 * PatternTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_
#define ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/os_utils.hpp>
#include <alphagomoku/utils/BitMask.hpp>

#include <array>
#include <map>
#include <cassert>
#include <iostream>

namespace ag
{
	enum class PatternType : int8_t
	{
		NONE,
		HALF_OPEN_3, // can become half open four in one move
		OPEN_3, // can become open four in one move
		HALF_OPEN_4, // can become a five in exactly one way
		OPEN_4, // can become a five in more than one way
		DOUBLE_4, // inline 4x4 fork
		FIVE, // actually, it is a winning pattern so for some rules it means 'five but not overline'
		OVERLINE
	};
	std::string toString(PatternType pt);

	struct PatternEncoding
	{
		private:
			uint32_t m_data = 0u;
		public:
			PatternEncoding() noexcept = default;
			PatternEncoding(PatternType cross, PatternType circle) noexcept :
					m_data(static_cast<uint32_t>(cross) | (static_cast<uint32_t>(circle) << 3))
			{
			}
			PatternType forCross() const noexcept
			{
				return static_cast<PatternType>(m_data & 7);
			}
			PatternType forCircle() const noexcept
			{
				return static_cast<PatternType>((m_data >> 3) & 7);
			}
			bool mustBeUpdated(int index) const noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				return static_cast<bool>((m_data >> index) & 1);
			}
			void setUpdateMask(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				m_data &= (~(1 << index));
				m_data |= (static_cast<uint32_t>(b) << index);
			}
			uint32_t getDefensiveMoves(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				return (sign == Sign::CROSS) ? ((m_data >> 17) & 127) : ((m_data >> 24) & 127);
			}
			void setDefensiveMoves(Sign sign, uint32_t maskIndex) noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				const uint32_t shift = (sign == Sign::CROSS) ? 17 : 24;
				m_data &= (~(127u << shift));
				m_data |= (maskIndex << shift);
			}
	};

	class PatternTable
	{
		private:
			std::vector<PatternEncoding> patterns;

			std::vector<BitMask1D<uint16_t>> defensive_move_mask;
			GameRules game_rules;
		public:
			PatternTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			PatternEncoding getPatternData(uint32_t pattern) const noexcept
			{
				assert(pattern < patterns.size());
				return patterns[pattern];
			}
			BitMask1D<uint16_t> getDefensiveMoves(PatternEncoding enc, Sign sign) const noexcept
			{
				return defensive_move_mask[enc.getDefensiveMoves(sign)];
			}
			static const PatternTable& get(GameRules rules);
		private:
			void init_features();
			void init_update_mask();
			void init_defensive_moves();
			size_t add_defensive_move_mask(BitMask1D<uint16_t> mask);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_ */
