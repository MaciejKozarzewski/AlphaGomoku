/*
 * ThreatTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_THREATTABLE_HPP_
#define ALPHAGOMOKU_PATTERNS_THREATTABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/common.hpp>

#include <cinttypes>

namespace ag
{
	enum class ThreatType : int8_t
	{
		NONE,
		HALF_OPEN_3,
		OPEN_3,
		FORK_3x3, // win in 5
		HALF_OPEN_4,
		FORK_4x3, // win in 5
		FORK_4x4, // win in 3
		OPEN_4, // win in 3
		FIVE, // win in 1
		OVERLINE
	};
	std::string toString(ThreatType t);

	struct ThreatEncoding
	{
		private:
			uint8_t data = 0u;
		public:
			ThreatEncoding() noexcept = default;
			ThreatEncoding(ThreatEncoding cross, ThreatEncoding circle) noexcept :
					data((cross.data & 0x0F) | (circle.data & 0xF0))
			{
			}
			ThreatEncoding(ThreatType cross, ThreatType circle) noexcept :
					data(static_cast<uint8_t>(cross) | (static_cast<uint8_t>(circle) << 4))
			{
			}
			ThreatEncoding(ThreatType tt) noexcept :
					ThreatEncoding(tt, tt)
			{
			}
			ThreatType forCross() const noexcept
			{
				return static_cast<ThreatType>(data & 0x0F);
			}
			ThreatType forCircle() const noexcept
			{
				return static_cast<ThreatType>(data >> 4);
			}
	};

	class ThreatTable
	{
		private:
			std::vector<ThreatEncoding> threats;
		public:
			ThreatTable(GameRules rules);
			ThreatEncoding getThreat(TwoPlayerGroup<PatternType> ptg) const noexcept
			{
				const uint32_t index_cross = get_index(ptg.for_cross);
				const uint32_t index_circle = get_index(ptg.for_circle);
				assert(index_cross < threats.size() && index_circle < threats.size());
				return ThreatEncoding(threats[index_cross], threats[index_circle]);
			}
			static const ThreatTable& get(GameRules rules);
		private:
			void init_threats(GameRules rules);
			constexpr uint32_t get_index(DirectionGroup<PatternType> group) const noexcept
			{
				return static_cast<uint32_t>(group.horizontal) + (static_cast<uint32_t>(group.vertical) << 3)
						+ (static_cast<uint32_t>(group.diagonal) << 6) + (static_cast<uint32_t>(group.antidiagonal) << 9);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_THREATTABLE_HPP_ */
