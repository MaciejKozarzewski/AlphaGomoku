/*
 * PatternTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_

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
			/*
			 * bits 0:3  - pattern type for cross
			 * bits 3:6  - pattern type for circle
			 * bits 6:16 - update mask (omitting central spot)
			 */
			uint16_t m_data = 0u;
		public:
			PatternEncoding() noexcept = default;
			PatternEncoding(PatternType cross, PatternType circle) noexcept :
					m_data(static_cast<uint16_t>(cross) | (static_cast<uint16_t>(circle) << 3))
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
				// central spot must always be updated so there is not stored
				return (index == 5) ? true : static_cast<bool>((m_data >> (index + 6 - static_cast<int>(index > 5))) & 1);
			}
			void setUpdateMask(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 11);
				if (index != 5)
				{
					index += 6 - static_cast<int>(index > 5);
					m_data &= (~(1 << index));
					m_data |= (static_cast<uint16_t>(b) << index);
				}
			}
	};

	class PatternTable
	{
		private:
			std::vector<PatternEncoding> pattern_types;
			GameRules game_rules;
		public:
			PatternTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			PatternEncoding getPatternData(uint32_t pattern) const noexcept
			{
				assert(pattern < pattern_types.size());
				return pattern_types[pattern];
			}
			static const PatternTable& get(GameRules rules);
		private:
			void init_features();
			void init_update_mask();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_ */
