/*
 * PatternTable_v2.hpp
 *
 *  Created on: Oct 3, 2022
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERNTABLE_V2_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERNTABLE_V2_HPP_

#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <array>
#include <map>
#include <cassert>
#include <iostream>

namespace ag::experimental
{

	struct PatternEncoding_v2
	{
		private:
			uint8_t m_data = 0u;
		public:
			PatternEncoding_v2() noexcept = default;
			PatternEncoding_v2(PatternType cross, PatternType circle) noexcept :
					m_data(static_cast<uint32_t>(cross) | (static_cast<uint32_t>(circle) << 3u))
			{
			}
			PatternType forCross() const noexcept
			{
				return static_cast<PatternType>(m_data & 7u);
			}
			PatternType forCircle() const noexcept
			{
				return static_cast<PatternType>(m_data >> 3u);
			}
	};

	struct DefensiveMovesEncoding
	{
		private:
			BitMask<uint16_t> m_cross;
			BitMask<uint16_t> m_circle;
		public:
			DefensiveMovesEncoding() noexcept = default;
			BitMask<uint16_t> get(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				return (sign == Sign::CROSS) ? m_cross : m_circle;
			}
			BitMask<uint16_t>& get(Sign sign) noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				return (sign == Sign::CROSS) ? m_cross : m_circle;
			}
	};

	class PatternTable_v2
	{
		private:
			std::vector<PatternEncoding_v2> pattern_types;
			std::vector<BitMask<uint16_t>> liberties;
			std::vector<DefensiveMovesEncoding> defensive_moves;
			GameRules game_rules;
		public:
			PatternTable_v2(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			PatternEncoding_v2 getPatternData(uint32_t pattern) const noexcept
			{
				assert(pattern < pattern_types.size());
				return pattern_types[pattern];
			}
			BitMask<uint16_t> getLiberties(uint32_t pattern) const noexcept
			{
				assert(pattern < liberties.size());
				return liberties[pattern];
			}
			BitMask<uint16_t> getDefensiveMoves(uint32_t pattern, Sign sign) const noexcept
			{
				assert(pattern < defensive_moves.size());
				return defensive_moves[pattern].get(sign);
			}
			static const PatternTable_v2& get(GameRules rules);
		private:
			void init_features();
			void init_liberties();
			void init_defensive_moves();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNTABLE_V2_HPP_ */
