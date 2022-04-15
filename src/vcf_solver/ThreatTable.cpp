/*
 * ThreatTable.cpp
 *
 *  Created on: Apr 9, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/ThreatTable.hpp>

#include <array>
#include <algorithm>

namespace
{
	using namespace ag;

	std::array<FeatureType, 4> make_feature_type_group(int f0, int f1, int f2, int f3) noexcept
	{
		std::array<FeatureType, 4> result;
		result[0] = static_cast<FeatureType>(f0);
		result[1] = static_cast<FeatureType>(f1);
		result[2] = static_cast<FeatureType>(f2);
		result[3] = static_cast<FeatureType>(f3);
		return result;
	}
	int count(std::array<FeatureType, 4> group, FeatureType ft) noexcept
	{
		return std::count(group.begin(), group.end(), ft);
	}
	bool contains(std::array<FeatureType, 4> group, FeatureType ft) noexcept
	{
		return count(group, ft) > 0;
	}

	bool is_five(std::array<FeatureType, 4> group) noexcept
	{
		return contains(group, FeatureType::FIVE);
	}
	bool is_overline(std::array<FeatureType, 4> group) noexcept
	{
		return contains(group, FeatureType::OVERLINE);
	}
	bool is_fork_3x3(std::array<FeatureType, 4> group) noexcept
	{
		return count(group, FeatureType::OPEN_3) >= 2;
	}
	bool is_fork_4x3(std::array<FeatureType, 4> group) noexcept
	{
		const int sum3 = count(group, FeatureType::OPEN_3);
		const int sum4 = count(group, FeatureType::OPEN_4) + count(group, FeatureType::HALF_OPEN_4);
		return sum3 >= 1 and sum4 >= 1;
	}
	bool is_fork_4x4(std::array<FeatureType, 4> group) noexcept
	{
		const int sum4 = count(group, FeatureType::OPEN_4) + count(group, FeatureType::HALF_OPEN_4);
		return contains(group, FeatureType::DOUBLE_4) or sum4 >= 2;
	}

	Threat_v3 get_threat(std::array<FeatureType, 4> group, GameRules rules) noexcept
	{
		if (is_five(group)) // win in 1
			return Threat_v3(ThreatType_v3::FIVE); // five is never forbidden, even in renju

		if (rules == GameRules::RENJU)
		{
			if (is_overline(group)) // forbidden or win in 1
				return Threat_v3(ThreatType_v3::FORBIDDEN, ThreatType_v3::FIVE);
			if (contains(group, FeatureType::OPEN_4)) // potentially win in 3
			{
				if (is_fork_4x4(group) or is_fork_3x3(group))
					return Threat_v3(ThreatType_v3::FORBIDDEN, ThreatType_v3::OPEN_4); // rare case when at the same spot there is an open four and a fork (in different directions)
				else
					return Threat_v3(ThreatType_v3::OPEN_4);
			}
			if (is_fork_4x4(group)) // forbidden or win in 3
				return Threat_v3(ThreatType_v3::FORBIDDEN, ThreatType_v3::FORK_4x4);
			if (is_fork_4x3(group)) // win in 5
				return Threat_v3(ThreatType_v3::FORK_4x3, ThreatType_v3::FORK_4x3);
			if (is_fork_3x3(group)) // forbidden or win in 5
				return Threat_v3(ThreatType_v3::FORBIDDEN, ThreatType_v3::FORK_3x3);  // TODO this requires recursive check for non-trivial 3x3 forks
		}
		else
		{
			if (contains(group, FeatureType::OPEN_4)) // win in 3
				return Threat_v3(ThreatType_v3::OPEN_4);
			if (is_fork_4x4(group)) // win in 3
				return Threat_v3(ThreatType_v3::FORK_4x4);
			if (is_fork_4x3(group)) // win in 5
				return Threat_v3(ThreatType_v3::FORK_4x3);
			if (is_fork_3x3(group)) // win in 5
				return Threat_v3(ThreatType_v3::FORK_3x3);
		}
		if (contains(group, FeatureType::HALF_OPEN_4))
			return Threat_v3(ThreatType_v3::HALF_OPEN_4);
		if (contains(group, FeatureType::OPEN_3))
			return Threat_v3(ThreatType_v3::OPEN_3);
		return Threat_v3(ThreatType_v3::NONE);
	}
}

namespace ag
{
	std::string toString(ThreatType_v3 t)
	{
		switch (t)
		{
			default:
			case ThreatType_v3::NONE:
				return "NONE";
			case ThreatType_v3::FORBIDDEN:
				return "FORBIDDEN";
			case ThreatType_v3::OPEN_3:
				return "OPEN_3";
			case ThreatType_v3::HALF_OPEN_4:
				return "HALF_OPEN_4";
			case ThreatType_v3::FORK_3x3:
				return "FORK_3x3";
			case ThreatType_v3::FORK_4x3:
				return "FORK_4x3";
			case ThreatType_v3::FORK_4x4:
				return "FORK_4x4";
			case ThreatType_v3::OPEN_4:
				return "OPEN_4";
			case ThreatType_v3::FIVE:
				return "FIVE";
		}
	}

	ThreatTable::ThreatTable(GameRules rules) :
			threats(8 * 8 * 8 * 8)
	{
		init_threats(rules);
	}
	/*
	 * private
	 */
	void ThreatTable::init_threats(GameRules rules)
	{
		for (int f0 = 0; f0 < 7; f0++)
			for (int f1 = 0; f1 < 7; f1++)
				for (int f2 = 0; f2 < 7; f2++)
					for (int f3 = 0; f3 < 7; f3++)
					{
						const std::array<FeatureType, 4> group = make_feature_type_group(f0, f1, f2, f3);
						const uint32_t index = f0 + (f1 << 3) + (f2 << 6) + (f3 << 9);
						const Threat_v3 t = get_threat(group, rules);
						threats[index] = t.encode();
					}
	}
} /* namespace ag */

