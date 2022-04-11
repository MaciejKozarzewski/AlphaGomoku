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

	class FeatureTypeGroup
	{
		private:
			FeatureType feature_types[4];
		public:
			FeatureType operator[](int dir) const noexcept
			{
				assert(dir >= 0 && dir < 4);
				return feature_types[dir];
			}
			FeatureType& operator[](int dir) noexcept
			{
				assert(dir >= 0 && dir < 4);
				return feature_types[dir];
			}
			uint32_t get(int dir) const noexcept
			{
				assert(dir >= 0 && dir < 4);
				return static_cast<uint32_t>(feature_types[dir]);
			}
			int count(FeatureType ft) const noexcept
			{
				int result = 0;
				for (int i = 0; i < 4; i++)
					result += static_cast<int>(feature_types[i] == ft);
				return result;
			}
			bool contains(FeatureType ft) const noexcept
			{
				for (int i = 0; i < 4; i++)
					if (feature_types[i] == ft)
						return true;
				return false;
			}
	};

	class ThreatTable
	{
		private:
			std::vector<uint8_t> threats;
		public:
			ThreatTable(GameRules rules);
			Threat_v3 getThreat(FeatureTypeGroup ftg) const noexcept
			{
				const uint32_t index = ftg.get(0) + (ftg.get(1) << 3) + (ftg.get(2) << 6) + (ftg.get(3) << 9);
				assert(index < threats.size());
				return Threat_v3(threats[index]);
			}
		private:
			void init_threats(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_THREATTABLE_HPP_ */
