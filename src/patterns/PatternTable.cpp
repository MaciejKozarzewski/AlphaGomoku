/*
 * PatternTable.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <cassert>
#include <map>
#include <algorithm>
#include <vector>
#include <array>
#include <cstring>
#include <bitset>

namespace
{
	using namespace ag;

	uint32_t invertColor(uint32_t pattern) noexcept
	{
		return ((pattern >> 1) & 0x55555555) | ((pattern & 0x55555555) << 1);
	}

	class ThreatClassifier
	{
			IsOverline is_overline;
			IsFive is_five;
			IsOpenFour is_open_four;
			IsDoubleFour is_double_four;
			IsHalfOpenFour is_half_open_four;
			IsOpenThree is_open_three;
			IsHalfOpenThree is_half_open_three;
		public:
			ThreatClassifier(GameRules rules, Sign sign) :
					is_overline(rules, sign),
					is_five(rules, sign),
					is_open_four(rules, sign),
					is_double_four(rules, sign),
					is_half_open_four(rules, sign),
					is_open_three(rules, sign),
					is_half_open_three(rules, sign)
			{
			}
			PatternType operator()(const Pattern &f) const noexcept
			{
				if (is_five(f))
					return PatternType::FIVE;
				if (is_overline(f))
					return PatternType::OVERLINE;
				if (is_open_four(f))
					return PatternType::OPEN_4;
				if (is_double_four(f))
					return PatternType::DOUBLE_4;
				if (is_half_open_four(f))
					return PatternType::HALF_OPEN_4;
				if (is_open_three(f))
					return PatternType::OPEN_3;
				if (is_half_open_three(f))
					return PatternType::HALF_OPEN_3;
				return PatternType::NONE;
			}
	};

	class DefensiveMoveFinder
	{
			const PatternTable &table;
			Sign defender_sign;
			IsOpenThree is_open_three;
		public:
			DefensiveMoveFinder(const PatternTable &t, Sign defenderSign) :
					table(t),
					defender_sign(defenderSign),
					is_open_three(t.getRules(), invertSign(defenderSign))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line)
			{
				const int expanded_size = (table.getRules() == GameRules::CARO) ? 4 : 2;
				const int offset = expanded_size / 2;
				line = Pattern(line.size() + expanded_size, line.encode() << expanded_size);
				BitMask1D<uint16_t> result;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						if (not is_open_three(line))
							result[i - offset] = true;
						line.set(i, Sign::NONE);
					}
				return result;
			}
	};

}

namespace ag
{
	std::string toString(PatternType pt)
	{
		switch (pt)
		{
			default:
			case PatternType::NONE:
				return "NONE";
			case PatternType::HALF_OPEN_3:
				return "HALF_OPEN_3";
			case PatternType::OPEN_3:
				return "OPEN_3";
			case PatternType::HALF_OPEN_4:
				return "HALF_OPEN_4";
			case PatternType::OPEN_4:
				return "OPEN_4";
			case PatternType::DOUBLE_4:
				return "DOUBLE_4";
			case PatternType::FIVE:
				return "FIVE";
			case PatternType::OVERLINE:
				return "OVERLINE";
		}
	}

	PatternTable::PatternTable(GameRules rules) :
			pattern_types(power(4, Pattern::length(rules))),
			game_rules(rules)
	{
		double t0 = getTime();
		init_features();
		double t1 = getTime();
		init_update_mask();
		double t2 = getTime();

//		std::cout << (t1 - t0) << " " << (t2 - t1) << std::endl;
//		std::cout << defensive_move_mask.size() << '\n';
//
//		std::cout << "five       : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::FIVE;}) << '\n';
//		std::cout << "double 4   : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::DOUBLE_4;}) << '\n';
//		std::cout << "open 4     : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::OPEN_4;}) << '\n';
//		std::cout << "half open 4: " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::HALF_OPEN_4;}) << '\n';
//		std::cout << "open 3     : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::OPEN_3;}) << '\n';
//		std::cout << "half open 3: " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
//		{	return enc.forCross() == PatternType::HALF_OPEN_3;}) << '\n' << '\n';
//
//
////		             "     C     "
//		Pattern line("|||_O_OOX||");
//		std::cout << line.encode() << '\n';
//		DefensiveMoveFinder2 dmf(*this, Sign::CIRCLE);
//		BitMask1D<uint16_t> res = dmf(line);
//
//		std::cout << "raw feature = " << line.encode() << '\n';
//		PatternEncoding data = getPatternData(line.encode());
//		std::cout << line.toString() << " " << toString(data.forCross()) << " " << toString(data.forCircle()) << '\n';
//		for (int i = 0; i < 11; i++)
//			std::cout << data.mustBeUpdated(i);
//		std::cout << " update mask\n";
//		for (int i = 0; i < 11; i++)
//			std::cout << getDefensiveMoves(data, Sign::CROSS)[i];
//		std::cout << " defensive moves for X\n";
//		for (int i = 0; i < 11; i++)
//			std::cout << getDefensiveMoves(data, Sign::CIRCLE)[i];
//		std::cout << " defensive moves for O\n";
//
//		for (int i = 0; i < 11; i++)
//			std::cout << res[i];
//		std::cout << " defensive moves for X v2\n";
//
//		SearchMoveFinder smf(*this, Sign::CIRCLE);
//		BitMask1D<uint16_t> mask = smf(line, 3);
//		for (int i = 0; i < 11; i++)
//			std::cout << mask[i];
//		std::cout << " defensive moves for O by search\n";
//		exit(0);
	}
	const PatternTable& PatternTable::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static const PatternTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static const PatternTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static const PatternTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static const PatternTable table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}
	/*
	 * private
	 */
	void PatternTable::init_features()
	{
		std::vector<bool> was_processed(pattern_types.size());

		const ThreatClassifier for_cross(game_rules, Sign::CROSS);
		const ThreatClassifier for_circle(game_rules, Sign::CIRCLE);

		Pattern line(Pattern::length(game_rules));

		for (size_t i = 0; i < pattern_types.size(); i++)
			if (was_processed[i] == false)
			{
				line.decode(i);
				if (line.isValid())
				{
					line.setCenter(Sign::CROSS);
					PatternType cross = for_cross(line);
					line.setCenter(Sign::CIRCLE);
					PatternType circle = for_circle(line);
					line.setCenter(Sign::NONE);

					pattern_types[i] = PatternEncoding(cross, circle);
					was_processed[i] = true;
					line.flip(); // the features are symmetrical so we can update both at the same time
					pattern_types[line.encode()] = PatternEncoding(cross, circle);
					was_processed[line.encode()] = true;
				}
			}
	}
	void PatternTable::init_update_mask()
	{
		std::vector<bool> was_processed(pattern_types.size());

		const int feature_length = Pattern::length(game_rules);
		const int side_length = feature_length / 2;
		Pattern base_line(feature_length);
		Pattern secondary_line(feature_length);

		for (size_t i = 0; i < pattern_types.size(); i++)
		{
			base_line.decode(i);
			if (was_processed[i] == false and base_line.isValid())
			{
				PatternEncoding pattern = getPatternData(i);
				for (int spot_index = 0; spot_index < feature_length; spot_index++)
					if (spot_index != side_length) // omitting central spot as it must always be updated anyway
					{
						const int free_spots = std::abs(side_length - spot_index);
						const int combinations = power(4, free_spots);

						base_line.decode(i);
						if (spot_index < side_length)
							base_line.shiftRight(free_spots);
						else
							base_line.shiftLeft(free_spots);

						bool must_be_updated = false;
						if (base_line.getCenter() == Sign::NONE)
							for (int j = 0; j < combinations; j++)
							{
								secondary_line.decode(j);
								if (spot_index > side_length)
									secondary_line.shiftRight(feature_length - free_spots);
								secondary_line.mergeWith(base_line);
								if (secondary_line.isValid())
								{
									const PatternEncoding original = getPatternData(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CROSS);
									const PatternEncoding cross_altered = getPatternData(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CIRCLE);
									const PatternEncoding circle_altered = getPatternData(secondary_line.encode());

									if (original.forCross() != cross_altered.forCross() or original.forCircle() != cross_altered.forCircle()
											or original.forCross() != circle_altered.forCross() or original.forCircle() != circle_altered.forCircle())
									{
										must_be_updated = true;
										break;
									}
								}
							}
						pattern.setUpdateMask(spot_index, must_be_updated);
					}
					else
						pattern.setUpdateMask(spot_index, true);

				pattern_types[i] = pattern;
				was_processed[i] = true;
				base_line.decode(i);

				// update mask is symmetrical so we can update both at the same time
				PatternEncoding flipped(pattern);
				for (int k = 0; k < feature_length; k++) // flip update mask
					flipped.setUpdateMask(k, pattern.mustBeUpdated(feature_length - 1 - k));

				base_line.flip();
				pattern_types[base_line.encode()] = flipped;
				was_processed[base_line.encode()] = true;
			}
		}
	}

} /* namespace ag */

