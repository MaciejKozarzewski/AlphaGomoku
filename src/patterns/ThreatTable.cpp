/*
 * ThreatTable.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/ThreatTable.hpp>

#include <array>
#include <stdexcept>
#include <algorithm>

namespace
{
	using namespace ag::experimental;

	std::array<PatternType, 4> make_feature_type_group(int f0, int f1, int f2, int f3) noexcept
	{
		std::array<PatternType, 4> result;
		result[0] = static_cast<PatternType>(f0);
		result[1] = static_cast<PatternType>(f1);
		result[2] = static_cast<PatternType>(f2);
		result[3] = static_cast<PatternType>(f3);
		return result;
	}
	int count(std::array<PatternType, 4> group, PatternType ft) noexcept
	{
		return std::count(group.begin(), group.end(), ft);
	}
	bool contains(std::array<PatternType, 4> group, PatternType ft) noexcept
	{
		return count(group, ft) > 0;
	}

	bool is_five(std::array<PatternType, 4> group) noexcept
	{
		return contains(group, PatternType::FIVE);
	}
	bool is_overline(std::array<PatternType, 4> group) noexcept
	{
		return contains(group, PatternType::OVERLINE);
	}
	bool is_fork_3x3(std::array<PatternType, 4> group) noexcept
	{
		return count(group, PatternType::OPEN_3) >= 2;
	}
	bool is_fork_4x3(std::array<PatternType, 4> group) noexcept
	{
		const int sum3 = count(group, PatternType::OPEN_3);
		const int sum4 = count(group, PatternType::OPEN_4) + count(group, PatternType::HALF_OPEN_4);
		return sum3 >= 1 and sum4 >= 1;
	}
	bool is_fork_4x4(std::array<PatternType, 4> group) noexcept
	{
		const int sum4 = count(group, PatternType::OPEN_4) + count(group, PatternType::HALF_OPEN_4);
		return contains(group, PatternType::DOUBLE_4) or sum4 >= 2;
	}

	Threat get_threat(std::array<PatternType, 4> group, ag::GameRules rules) noexcept
	{
		if (is_five(group)) // win in 1
			return Threat(ThreatType::FIVE); // five is never forbidden, even in renju

		if (rules == ag::GameRules::RENJU)
		{
			if (is_overline(group)) // forbidden or win in 1
				return Threat(ThreatType::OVERLINE, ThreatType::FIVE);
			if (is_fork_4x4(group)) // win in 3
				return Threat(ThreatType::FORK_4x4);
			if (contains(group, PatternType::OPEN_4)) // potentially win in 3
			{
				if (is_fork_3x3(group))
					return Threat(ThreatType::FORK_3x3, ThreatType::OPEN_4); // rare case when at the same spot there is an open four and a fork (in different directions)
				return Threat(ThreatType::OPEN_4);
			}
			if (is_fork_4x3(group)) // win in 5
			{
				if (is_fork_3x3(group))
					return Threat(ThreatType::FORK_3x3, ThreatType::FORK_4x3); // in renju, despite the 4x3 fork being allowed, if there is also 3x3 fork the entire move is forbidden
				else
					return Threat(ThreatType::FORK_4x3);
			}
		}
		else
		{
			if (is_fork_4x4(group)) // win in 3
				return Threat(ThreatType::FORK_4x4);
			if (contains(group, PatternType::OPEN_4)) // win in 3
				return Threat(ThreatType::OPEN_4);
			if (is_fork_4x3(group)) // win in 5
				return Threat(ThreatType::FORK_4x3);
		}
		if (is_fork_3x3(group)) // win in 5
			return Threat(ThreatType::FORK_3x3);
		if (contains(group, PatternType::HALF_OPEN_4))
			return Threat(ThreatType::HALF_OPEN_4);
		if (contains(group, PatternType::OPEN_3))
			return Threat(ThreatType::OPEN_3);
		if (contains(group, PatternType::HALF_OPEN_3))
			return Threat(ThreatType::HALF_OPEN_3);
		return Threat(ThreatType::NONE);
	}
}

namespace ag::experimental
{
	std::string toString(ThreatType t)
	{
		switch (t)
		{
			default:
			case ThreatType::NONE:
				return "NONE";
			case ThreatType::HALF_OPEN_3:
				return "HALF_OPEN_3";
			case ThreatType::OPEN_3:
				return "OPEN_3";
			case ThreatType::HALF_OPEN_4:
				return "HALF_OPEN_4";
			case ThreatType::FORK_3x3:
				return "FORK_3x3";
			case ThreatType::FORK_4x3:
				return "FORK_4x3";
			case ThreatType::FORK_4x4:
				return "FORK_4x4";
			case ThreatType::OPEN_4:
				return "OPEN_4";
			case ThreatType::FIVE:
				return "FIVE";
			case ThreatType::OVERLINE:
				return "OVERLINE";
		}
	}

	ThreatTable::ThreatTable(GameRules rules) :
			threats(8 * 8 * 8 * 8)
	{
		init_threats(rules);
	}
	const ThreatTable& ThreatTable::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static ThreatTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static ThreatTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static ThreatTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static ThreatTable table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}
	/*
	 * private
	 */
	void ThreatTable::init_threats(GameRules rules)
	{
		for (int f0 = 0; f0 < 8; f0++)
			for (int f1 = 0; f1 < 8; f1++)
				for (int f2 = 0; f2 < 8; f2++)
					for (int f3 = 0; f3 < 8; f3++)
					{
						const std::array<PatternType, 4> group = make_feature_type_group(f0, f1, f2, f3);
						threats[get_index(group)] = get_threat(group, rules);
					}
	}
} /* namespace ag */

