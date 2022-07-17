/*
 * PatternTable.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/solver/PatternTable.hpp>
#include <alphagomoku/solver/PatternClassifier.hpp>
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
//				if (is_half_open_three(f))
//					return PatternType::HALF_OPEN_3;
				return PatternType::NONE;
			}
	};

	class DefensiveMoveFinder
	{
			const PatternTable &table;
			IsFive is_five;
			int check_outcome(Pattern line, Sign signToMove, int depth)
			{
				if (is_five(line))
					return 1; // win found

				bool all_losing = true;
				bool at_least_one = false;
				for (size_t i = 1; i < line.size() - 1; i++)
					if (line.get(i) == Sign::NONE)
					{
						at_least_one = true;
						line.set(i, signToMove);
						int tmp = (depth == 7) ? 0 : check_outcome(line, invertSign(signToMove), depth + 1);
						line.set(i, Sign::NONE);

						all_losing &= (tmp == -1);
						if (tmp == 1)
							return -1;
					}
				if (all_losing and at_least_one)
					return 1;
				else
					return 0;
			}
		public:
			DefensiveMoveFinder(const PatternTable &t, Sign sign) :
					table(t),
					is_five(t.getRules(), invertSign(sign))
			{
			}
			BitMask<uint16_t> operator()(Pattern line)
			{
				line = Pattern(line.size() + 2, line.encode() << 2);
				BitMask<uint16_t> result;
				const Sign defender_sign = invertSign(is_five.getSign());
				for (size_t i = 1; i < line.size() - 1; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						int tmp = check_outcome(line, invertSign(defender_sign), 0);
						if (tmp != -1)
							result.set(i - 1, true);
						line.set(i, Sign::NONE);
					}
				return result;
			}
	};
}

namespace ag::experimental
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
			patterns(power(4, Pattern::length(rules))),
			game_rules(rules)
	{
		double t0 = getTime();
		init_features();
		double t1 = getTime();
		init_update_mask();
		double t2 = getTime();
		init_defensive_moves();
		double t3 = getTime();

//		std::cout << (t1 - t0) << " " << (t2 - t1) << " " << (t3 - t2) << std::endl;
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
//		Pattern line("___OO______");
//		DefensiveMoveFinder dmf(*this, Sign::CROSS);
//		BitMask<uint16_t> res = dmf(line);
//
//		std::cout << "raw feature = " << line.encode() << '\n';
//		PatternEncoding data = getPatternData(line.encode());
//		std::cout << line.toString() << " " << toString(data.forCross()) << " " << toString(data.forCircle()) << '\n';
//		for (int i = 0; i < 11; i++)
//			std::cout << data.mustBeUpdated(i);
//		std::cout << '\n';
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
	const PatternTable& PatternTable::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static PatternTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static PatternTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static PatternTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static PatternTable table(GameRules::CARO);
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
		std::vector<bool> was_processed(patterns.size());

		const ThreatClassifier for_cross(game_rules, Sign::CROSS);
		const ThreatClassifier for_circle(game_rules, Sign::CIRCLE);

		Pattern line(Pattern::length(game_rules));

		for (size_t i = 0; i < patterns.size(); i++)
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

					patterns[i] = PatternEncoding(cross, circle);
					was_processed[i] = true;
					line.flip(); // the features are symmetrical so we can update both at the same time
					patterns[line.encode()] = PatternEncoding(cross, circle);
					was_processed[line.encode()] = true;
				}
			}
	}
	void PatternTable::init_update_mask()
	{
		std::vector<bool> was_processed(patterns.size());

		const int feature_length = Pattern::length(game_rules);
		const int side_length = feature_length / 2;
		Pattern base_line(feature_length);
		Pattern secondary_line(feature_length);

		for (size_t i = 0; i < patterns.size(); i++)
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

				patterns[i] = pattern;
				was_processed[i] = true;
				base_line.decode(i);

				// update mask is symmetrical so we can update both at the same time
				PatternEncoding flipped(pattern);
				for (int k = 0; k < feature_length; k++) // flip update mask
					flipped.setUpdateMask(k, pattern.mustBeUpdated(feature_length - 1 - k));

				base_line.flip();
				patterns[base_line.encode()] = flipped;
				was_processed[base_line.encode()] = true;
			}
		}
	}
	void PatternTable::init_defensive_moves()
	{
		std::vector<bool> was_processed(patterns.size());

		defensive_move_mask.reserve(128);
		const int feature_length = Pattern::length(game_rules);
		Pattern base_line(feature_length);
		Pattern secondary_line(feature_length);

		DefensiveMoveFinder dmf_cross(*this, Sign::CROSS);
		DefensiveMoveFinder dmf_circle(*this, Sign::CIRCLE);

		for (size_t i = 0; i < patterns.size(); i++)
		{
			base_line.decode(i);
			if (was_processed[i] == false and base_line.isValid())
			{
				BitMask<uint16_t> mask_cross, mask_circle;
				PatternEncoding feature = getPatternData(i);
				for (int spot_index = 0; spot_index < feature_length; spot_index++)
					if (base_line.get(spot_index) == Sign::NONE)
					{
						base_line.set(spot_index, Sign::CROSS);
						const PatternEncoding cross_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::CIRCLE);
						const PatternEncoding circle_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::NONE);

						if (cross_refuted.forCircle() < feature.forCircle())
							mask_cross.set(spot_index, true);
						if (circle_refuted.forCross() < feature.forCross())
							mask_circle.set(spot_index, true);
					}
				if (feature.forCircle() == PatternType::OPEN_4)
					mask_cross &= dmf_cross(base_line);
				if (feature.forCross() == PatternType::OPEN_4)
					mask_circle &= dmf_circle(base_line);

				patterns[i].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(mask_cross));
				patterns[i].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(mask_circle));
				was_processed[i] = true;

				base_line.decode(i);
				base_line.flip();
				mask_cross.flip(feature_length);
				mask_circle.flip(feature_length);
				patterns[base_line.encode()].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(mask_cross));
				patterns[base_line.encode()].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(mask_circle));
				was_processed[base_line.encode()] = true;
			}
		}
		assert(defensive_move_mask.size() < 128); // only 7 bits are reserved in PatternEncoding for storing index of defensive move mask
	}
	size_t PatternTable::add_defensive_move_mask(BitMask<uint16_t> mask)
	{
		if (std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask) == defensive_move_mask.end())
			defensive_move_mask.push_back(mask);
		return std::distance(defensive_move_mask.begin(), std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask));
	}

} /* namespace ag */

