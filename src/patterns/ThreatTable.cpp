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

#include <iostream>

namespace
{
	using namespace ag;

	DirectionGroup<PatternType> make_pattern_type_group(int f0, int f1, int f2, int f3) noexcept
	{
		DirectionGroup<PatternType> result;
		result.horizontal = static_cast<PatternType>(f0);
		result.vertical = static_cast<PatternType>(f1);
		result.diagonal = static_cast<PatternType>(f2);
		result.antidiagonal = static_cast<PatternType>(f3);
		return result;
	}

	bool is_five(DirectionGroup<PatternType> group) noexcept
	{
		return group.contains(PatternType::FIVE);
	}
	bool is_overline(DirectionGroup<PatternType> group) noexcept
	{
		return group.contains(PatternType::OVERLINE);
	}
	bool is_fork_3x3(DirectionGroup<PatternType> group) noexcept
	{
		return group.count(PatternType::OPEN_3) >= 2;
	}
	bool is_fork_4x3(DirectionGroup<PatternType> group) noexcept
	{
		const int sum3 = group.count(PatternType::OPEN_3);
		const int sum4 = group.count(PatternType::OPEN_4) + group.count(PatternType::HALF_OPEN_4);
		return sum3 >= 1 and sum4 >= 1;
	}
	bool is_fork_4x4(DirectionGroup<PatternType> group) noexcept
	{
		const int sum4 = group.count(PatternType::OPEN_4) + group.count(PatternType::HALF_OPEN_4);
		return group.contains(PatternType::DOUBLE_4) or sum4 >= 2;
	}

	ThreatEncoding get_threat(DirectionGroup<PatternType> group, GameRules rules) noexcept
	{
		if (is_five(group)) // win in 1
			return ThreatEncoding(ThreatType::FIVE); // five is never forbidden, even in renju

		if (rules == ag::GameRules::RENJU)
		{
			if (is_overline(group)) // forbidden or win in 1
				return ThreatEncoding(ThreatType::OVERLINE, ThreatType::FIVE);
			if (is_fork_4x4(group)) // win in 3
				return ThreatEncoding(ThreatType::FORK_4x4);
			if (group.contains(PatternType::OPEN_4)) // potentially win in 3
			{
				if (is_fork_3x3(group)) // rare case when at the same spot there is an open four and a fork (in different directions)
					return ThreatEncoding(ThreatType::FORK_3x3, ThreatType::OPEN_4);
				else
					return ThreatEncoding(ThreatType::OPEN_4);
			}
			if (is_fork_4x3(group)) // win in 5
			{
				if (is_fork_3x3(group)) // in renju, despite the 4x3 fork being allowed, if there is also 3x3 fork the entire move is forbidden
					return ThreatEncoding(ThreatType::FORK_3x3, ThreatType::FORK_4x3);
				else
					return ThreatEncoding(ThreatType::FORK_4x3);
			}
		}
		else
		{
			if (is_fork_4x4(group)) // win in 3
				return ThreatEncoding(ThreatType::FORK_4x4);
			if (group.contains(PatternType::OPEN_4)) // win in 3
				return ThreatEncoding(ThreatType::OPEN_4);
			if (is_fork_4x3(group)) // win in 5
				return ThreatEncoding(ThreatType::FORK_4x3);
		}
		if (is_fork_3x3(group)) // win in 5
			return ThreatEncoding(ThreatType::FORK_3x3);
		if (group.contains(PatternType::HALF_OPEN_4))
			return ThreatEncoding(ThreatType::HALF_OPEN_4);
		if (group.contains(PatternType::OPEN_3))
			return ThreatEncoding(ThreatType::OPEN_3);
		if (group.contains(PatternType::HALF_OPEN_3))
			return ThreatEncoding(ThreatType::HALF_OPEN_3);
		return ThreatEncoding(ThreatType::NONE);
	}
}

namespace ag
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
			threats(8 * 8 * 8 * 8),
			rules(rules)
	{
		init_threats();
	}
	const ThreatTable& ThreatTable::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static const ThreatTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static const ThreatTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static const ThreatTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO5:
			{
				static const ThreatTable table(GameRules::CARO5);
				return table;
			}
			case GameRules::CARO6:
			{
				static const ThreatTable table(GameRules::CARO6);
				return table;
			}
			default:
				throw std::logic_error("ThreatTable::get() unknown rule " + std::to_string((int) rules));
		}
	}
	/*
	 * private
	 */
	void ThreatTable::init_threats()
	{
		for (int f0 = 0; f0 < 8; f0++)
			for (int f1 = 0; f1 < 8; f1++)
				for (int f2 = 0; f2 < 8; f2++)
					for (int f3 = 0; f3 < 8; f3++)
					{
						const DirectionGroup<PatternType> group = make_pattern_type_group(f0, f1, f2, f3);
						threats[get_index(group)] = get_threat(group, rules);
					}
	}
} /* namespace ag */

