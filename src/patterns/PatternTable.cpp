/*
 * PatternTable.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/utils/BitMask.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstring>

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
//				if (is_half_open_three(f))
//					return PatternType::HALF_OPEN_3;
				return PatternType::NONE;
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
			pattern_types(power(4, Pattern::length - 1)),
			update_mask(power(4, Pattern::length - 1)),
			game_rules(rules)
	{
//		double t0 = getTime();
		init_features();
//		double t1 = getTime();
		init_update_mask();
//		double t2 = getTime();
//		std::cout << (t1 - t0) << " " << (t2 - t1) << '\n';
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
			case GameRules::CARO5:
			{
				static const PatternTable table(GameRules::CARO5);
				return table;
			}
			case GameRules::CARO6:
			{
				static const PatternTable table(GameRules::CARO6);
				return table;
			}
			default:
				throw std::logic_error("PatternTable::get() unknown rule " + std::to_string((int) rules));
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

		Pattern line(Pattern::length);

		for (size_t i = 0; i < pattern_types.size(); i++)
			if (was_processed[i] == false)
			{
				line.decode(expand(i));
				if (line.isValid())
				{
					line.setCenter(Sign::CROSS);
					const PatternType cross = for_cross(line);
					line.setCenter(Sign::CIRCLE);
					const PatternType circle = for_circle(line);
					line.setCenter(Sign::NONE);

					pattern_types[i] = PatternEncoding(cross, circle);
					was_processed[i] = true;
					line.flip(); // the features are symmetrical so we can update both at the same time
					const int idx = line.encode();
					pattern_types[narrow_down(idx)] = PatternEncoding(cross, circle);
					was_processed[narrow_down(idx)] = true;
				}
			}
	}
	void PatternTable::init_update_mask()
	{
		std::vector<BitMask1D<uint16_t>> unique_masks;

		std::vector<bool> was_processed(update_mask.size());

		const int side_length = (Pattern::length - 1) / 2;
		Pattern base_line(Pattern::length);
		Pattern secondary_line(Pattern::length);

		for (size_t i = 0; i < update_mask.size(); i++)
		{
			const uint32_t idx = expand(i);
			base_line.decode(idx);
			if (was_processed[i] == false and base_line.isValid())
			{
				UpdateMask mask_cross_altered, mask_circle_altered;
				mask_cross_altered.set(side_length, true, true);
				mask_circle_altered.set(side_length, true, true);
				for (int spot_index = 0; spot_index < Pattern::length; spot_index++)
					if (spot_index != side_length) // omitting central spot as it must always be updated anyway
					{
						const int free_spots = std::abs(side_length - spot_index);
						const int combinations = power(4, free_spots);

						base_line.decode(idx);
						if (spot_index < side_length)
							base_line.shiftRight(free_spots);
						else
							base_line.shiftLeft(free_spots);

						bool cross_flag = false, circle_flag = false;
						bool cross_flag2 = false, circle_flag2 = false;
						if (base_line.getCenter() == Sign::NONE)
						{
							for (int j = 0; j < combinations; j++)
							{
								secondary_line.decode(j);
								if (spot_index > side_length)
									secondary_line.shiftRight(Pattern::length - free_spots);
								secondary_line.mergeWith(base_line);
								if (secondary_line.isValid())
								{
									const PatternEncoding original = getPatternType(NormalPattern(secondary_line.encode()));
									secondary_line.set(Pattern::length - 1 - spot_index, Sign::CROSS);
									const PatternEncoding cross_altered = getPatternType(NormalPattern(secondary_line.encode()));
									secondary_line.set(Pattern::length - 1 - spot_index, Sign::CIRCLE);
									const PatternEncoding circle_altered = getPatternType(NormalPattern(secondary_line.encode()));

									if (original.forCross() != cross_altered.forCross())
										cross_flag = true;
									if (original.forCircle() != cross_altered.forCircle())
										circle_flag = true;

									if (original.forCross() != circle_altered.forCross())
										cross_flag2 = true;
									if (original.forCircle() != circle_altered.forCircle())
										circle_flag2 = true;

									if (cross_flag and circle_flag and cross_flag2 and circle_flag2)
										break;
								}
							}
						}
						mask_cross_altered.set(spot_index, cross_flag, circle_flag);
						mask_circle_altered.set(spot_index, cross_flag2, circle_flag2);
					}

				update_mask[i][0] = mask_cross_altered;
				update_mask[i][1] = mask_circle_altered;

				was_processed[i] = true;
				base_line.decode(idx);

				// update mask is symmetrical so we can update both at the same time
				mask_cross_altered.flip(Pattern::length);
				mask_circle_altered.flip(Pattern::length);

				base_line.flip();
				update_mask[narrow_down(base_line.encode())][0] = mask_cross_altered;
				update_mask[narrow_down(base_line.encode())][1] = mask_circle_altered;
				was_processed[narrow_down(base_line.encode())] = true;
			}
		}
	}

} /* namespace ag */

