/*
 * ThreatTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_THREATTABLE_HPP_
#define ALPHAGOMOKU_SOLVER_THREATTABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/solver/PatternTable.hpp>

#include <cinttypes>

namespace ag::experimental
{
	enum class ThreatType : int8_t
	{
		NONE,
		FORBIDDEN,
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

	struct Threat
	{
		private:
			uint8_t data = 0u;
		public:
			Threat() noexcept = default;
			Threat(Threat encoding_cross, Threat encoding_circle) noexcept :
					data((encoding_cross.data & 15) | (encoding_circle.data & 240))
			{
			}
			Threat(ThreatType cross, ThreatType circle) noexcept :
					data(static_cast<uint8_t>(cross) | (static_cast<uint8_t>(circle) << 4))
			{
			}
			Threat(ThreatType tt) noexcept :
					Threat(tt, tt)
			{
			}
			ThreatType forCross() const noexcept
			{
				return static_cast<ThreatType>(data & 15);
			}
			ThreatType forCircle() const noexcept
			{
				return static_cast<ThreatType>((data >> 4) & 15);
			}
	};

	constexpr uint32_t get_index(std::array<PatternType, 4> group) noexcept
	{
		return static_cast<uint32_t>(group[0]) + (static_cast<uint32_t>(group[1]) << 3) + (static_cast<uint32_t>(group[2]) << 6)
				+ (static_cast<uint32_t>(group[3]) << 9);
	}

	class ThreatTable
	{
		private:
			std::vector<Threat> threats;
		public:
			ThreatTable(GameRules rules);
			Threat getThreat(PatternTypeGroup ptg) const noexcept
			{
				const uint32_t index_cross = get_index(ptg.for_cross);
				const uint32_t index_circle = get_index(ptg.for_circle);
				assert(index_cross < threats.size() && index_circle < threats.size());
				return Threat(threats[index_cross], threats[index_circle]);
			}
			static const ThreatTable& get(GameRules rules);
		private:
			void init_threats(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_THREATTABLE_HPP_ */
