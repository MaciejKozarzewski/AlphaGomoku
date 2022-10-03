/*
 * PatternTable_v2.cpp
 *
 *  Created on: Oct 3, 2022
 *      Author: maciek
 */

#include <alphagomoku/patterns/PatternTable_v2.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <cassert>
#include <map>
#include <algorithm>
#include <vector>
#include <array>
#include <cstring>

namespace
{
	using namespace ag;
	using namespace ag::experimental;

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
			const PatternTable_v2 &table;
			Sign defender_sign;
			IsOpenThree is_open_three;
		public:
			DefensiveMoveFinder(const PatternTable_v2 &t, Sign defenderSign) :
					table(t),
					defender_sign(defenderSign),
					is_open_three(t.getRules(), invertSign(defenderSign))
			{
			}
			BitMask<uint16_t> operator()(Pattern line)
			{
				const int expanded_size = (table.getRules() == GameRules::CARO) ? 4 : 2;
				const int offset = expanded_size / 2;
				line = Pattern(line.size() + expanded_size, line.encode() << expanded_size);
				BitMask<uint16_t> result;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						if (not is_open_three(line))
							result.set(i - offset, true);
						line.set(i, Sign::NONE);
					}
				return result;
			}
	};
}

namespace ag::experimental
{
	PatternTable_v2::PatternTable_v2(GameRules rules) :
			pattern_types(power(4, Pattern::length(rules))),
			liberties(pattern_types.size()),
			defensive_moves(pattern_types.size()),
			game_rules(rules)
	{
		double t0 = getTime();
		init_features();
		double t1 = getTime();
		init_liberties();
		double t2 = getTime();
		init_defensive_moves();
		double t3 = getTime();

		std::cout << (t1 - t0) << " " << (t2 - t1) << " " << (t3 - t2) << std::endl;

		std::cout << "five       : " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::FIVE;}) << '\n';
		std::cout << "double 4   : " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::DOUBLE_4;}) << '\n';
		std::cout << "open 4     : " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::OPEN_4;}) << '\n';
		std::cout << "half open 4: " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::HALF_OPEN_4;}) << '\n';
		std::cout << "open 3     : " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::OPEN_3;}) << '\n';
		std::cout << "half open 3: " << std::count_if(pattern_types.begin(), pattern_types.end(), [](const PatternEncoding_v2 &enc)
		{	return enc.forCross() == PatternType::HALF_OPEN_3;}) << '\n' << '\n';
//
//		Pattern line("__OOO______");
//		DefensiveMoveFinder dmf(*this, Sign::CROSS);
//		BitMask<uint16_t> res = dmf(line);
//
//		std::cout << "raw feature = " << line.encode() << '\n';
//		PatternEncoding data = getPatternData(line.encode());
//		std::cout << line.toString() << " " << toString(data.forCross()) << " " << toString(data.forCircle()) << '\n';
//		for (int i = 0; i < 11; i++)
//			std::cout << data.mustBeUpdated(i);
//		std::cout << " update mask\n";
//		for (int i = 0; i < 11; i++)
//			std::cout << getDefensiveMoves(data, Sign::CROSS).get(i);
//		std::cout << " defensive moves for X\n";
//		for (int i = 0; i < 11; i++)
//			std::cout << getDefensiveMoves(data, Sign::CIRCLE).get(i);
//		std::cout << " defensive moves for O\n";
//
//		for (int i = 0; i < 11; i++)
//			std::cout << res.get(i);
//		std::cout << " defensive moves for X v2\n";
//		exit(0);
	}
	const PatternTable_v2& PatternTable_v2::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static PatternTable_v2 table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static PatternTable_v2 table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static PatternTable_v2 table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static PatternTable_v2 table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}
	/*
	 * private
	 */
	void PatternTable_v2::init_features()
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

					pattern_types[i] = PatternEncoding_v2(cross, circle);
					was_processed[i] = true;
					line.flip(); // the features are symmetrical so we can update both at the same time
					pattern_types[line.encode()] = PatternEncoding_v2(cross, circle);
					was_processed[line.encode()] = true;
				}
			}
	}
	void PatternTable_v2::init_liberties()
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
				BitMask<uint16_t> update_mask;
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
									const PatternEncoding_v2 original = getPatternData(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CROSS);
									const PatternEncoding_v2 cross_altered = getPatternData(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CIRCLE);
									const PatternEncoding_v2 circle_altered = getPatternData(secondary_line.encode());

									if (original.forCross() != cross_altered.forCross() or original.forCircle() != cross_altered.forCircle()
											or original.forCross() != circle_altered.forCross() or original.forCircle() != circle_altered.forCircle())
									{
										must_be_updated = true;
										break;
									}
								}
							}
						update_mask.set(spot_index, must_be_updated);
					}
					else
						update_mask.set(spot_index, true);

				liberties[i] = update_mask;
				was_processed[i] = true;
				base_line.decode(i);

				// update mask is symmetrical so we can update both at the same time
				update_mask.flip(feature_length);

				base_line.flip();
				liberties[base_line.encode()] = update_mask;
				was_processed[base_line.encode()] = true;
			}
		}
	}
	void PatternTable_v2::init_defensive_moves()
	{
		std::vector<bool> was_processed(pattern_types.size());

		const int feature_length = Pattern::length(game_rules);
		Pattern base_line(feature_length);
		Pattern secondary_line(feature_length);

		DefensiveMoveFinder dmf_cross(*this, Sign::CROSS);
		DefensiveMoveFinder dmf_circle(*this, Sign::CIRCLE);

		for (size_t i = 0; i < pattern_types.size(); i++)
		{
			base_line.decode(i);
			if (was_processed[i] == false and base_line.isValid())
			{
				BitMask<uint16_t> mask_cross, mask_circle;
				PatternEncoding_v2 feature = getPatternData(i);
				for (int spot_index = 0; spot_index < feature_length; spot_index++)
					if (base_line.get(spot_index) == Sign::NONE)
					{
						base_line.set(spot_index, Sign::CROSS);
						const PatternEncoding_v2 cross_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::CIRCLE);
						const PatternEncoding_v2 circle_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::NONE);

						if (cross_refuted.forCircle() < feature.forCircle())
							mask_cross.set(spot_index, true);
						if (circle_refuted.forCross() < feature.forCross())
							mask_circle.set(spot_index, true);
					}
				if (feature.forCircle() == PatternType::OPEN_4)
					mask_cross = dmf_cross(base_line);
				if (feature.forCross() == PatternType::OPEN_4)
					mask_circle = dmf_circle(base_line);

				defensive_moves[i].get(Sign::CROSS) = mask_cross;
				defensive_moves[i].get(Sign::CIRCLE) = mask_circle;
				was_processed[i] = true;

				base_line.decode(i);
				base_line.flip();
				mask_cross.flip(feature_length);
				mask_circle.flip(feature_length);
				defensive_moves[base_line.encode()].get(Sign::CROSS) = mask_cross;
				defensive_moves[base_line.encode()].get(Sign::CIRCLE) = mask_circle;
				was_processed[base_line.encode()] = true;
			}
		}
	}

} /* namespace ag */

