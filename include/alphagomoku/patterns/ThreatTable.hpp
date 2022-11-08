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

	class ThreatEncoding
	{
		private:
			ThreatType for_cross = ThreatType::NONE;
			ThreatType for_circle = ThreatType::NONE;
		public:
			ThreatEncoding() noexcept = default;
			ThreatEncoding(ThreatEncoding cross, ThreatEncoding circle) noexcept :
					for_cross(cross.for_cross),
					for_circle(circle.for_circle)
			{
			}
			ThreatEncoding(ThreatType cross, ThreatType circle) noexcept :
					for_cross(cross),
					for_circle(circle)
			{
			}
			ThreatEncoding(ThreatType tt) noexcept :
					ThreatEncoding(tt, tt)
			{
			}
			ThreatType forCross() const noexcept
			{
				return for_cross;
			}
			ThreatType forCircle() const noexcept
			{
				return for_circle;
			}
			void setForCross(ThreatType tt) noexcept
			{
				for_cross = tt;
			}
			void setForCircle(ThreatType tt) noexcept
			{
				for_circle = tt;
			}
	};

	class ThreatTable
	{
		private:
			std::vector<ThreatEncoding> threats;
			GameRules rules;
		public:
			ThreatTable(GameRules rules);
			ThreatEncoding getThreat(TwoPlayerGroup<DirectionGroup<PatternType>> ptg) const noexcept
			{
				const uint32_t index_cross = get_index(ptg.for_cross);
				const uint32_t index_circle = get_index(ptg.for_circle);
				assert(index_cross < threats.size() && index_circle < threats.size());
				return ThreatEncoding(threats[index_cross], threats[index_circle]);
			}
			template<Sign S>
			ThreatType getThreat(DirectionGroup<PatternType> group) const noexcept
			{
				const ThreatEncoding tmp = threats[get_index(group)];
				return (S == Sign::CROSS) ? tmp.forCross() : tmp.forCircle();
			}
			static const ThreatTable& get(GameRules rules);
		private:
			void init_threats();
			constexpr uint32_t get_index(DirectionGroup<PatternType> group) const noexcept
			{
				return static_cast<uint32_t>(group.horizontal) + (static_cast<uint32_t>(group.vertical) << 3)
						+ (static_cast<uint32_t>(group.diagonal) << 6) + (static_cast<uint32_t>(group.antidiagonal) << 9);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_THREATTABLE_HPP_ */
