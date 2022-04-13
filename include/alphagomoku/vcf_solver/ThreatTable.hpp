/*
 * ThreatTable.hpp
 *
 *  Created on: Apr 9, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_THREATTABLE_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_THREATTABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/vcf_solver/FeatureTable_v3.hpp>

#include <cinttypes>

namespace ag
{
	enum class ThreatType_v3 : int8_t
	{
		NONE,
		OPEN_3,
		HALF_OPEN_4,
		OPEN_4,
		FIVE,
		FORBIDDEN,
		FORK_3x3,
		FORK_4x3,
		FORK_4x4
	};

	std::string toString(ThreatType_v3 t);

	struct Threat_v3
	{
			ThreatType_v3 for_cross = ThreatType_v3::NONE;
			ThreatType_v3 for_circle = ThreatType_v3::NONE;

			Threat_v3() noexcept = default;
			Threat_v3(uint8_t encoding) :
					for_cross(static_cast<ThreatType_v3>(encoding & 15)),
					for_circle(static_cast<ThreatType_v3>((encoding >> 4) & 15))
			{
			}
			Threat_v3(ThreatType_v3 tt) noexcept :
					for_cross(tt),
					for_circle(tt)
			{
			}
			Threat_v3(ThreatType_v3 cross, ThreatType_v3 circle) noexcept :
					for_cross(cross),
					for_circle(circle)
			{
			}
			uint8_t encode() const noexcept
			{
				return static_cast<uint32_t>(for_cross) | (static_cast<uint32_t>(for_circle) << 4);
			}
	};

	constexpr uint32_t get_index(std::array<FeatureType, 4> group) noexcept
	{
		return static_cast<uint32_t>(group[0]) + (static_cast<uint32_t>(group[1]) << 3) + (static_cast<uint32_t>(group[2]) << 6)
				+ (static_cast<uint32_t>(group[3]) << 9);
	}

	class ThreatTable
	{
		private:
			std::vector<uint8_t> threats;
		public:
			ThreatTable(GameRules rules);
			Threat_v3 getThreat(FeatureTypeGroup ftg) const noexcept
			{
				const uint32_t index_cross = get_index(ftg.for_cross);
				const uint32_t index_circle = get_index(ftg.for_circle);
				assert(index_cross < threats.size() && index_circle < threats.size());
				Threat_v3 threat_cross(threats[index_cross]);
				Threat_v3 threat_circle(threats[index_circle]);
				return Threat_v3(threat_cross.for_cross, threat_circle.for_circle);
			}
		private:
			void init_threats(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_THREATTABLE_HPP_ */
