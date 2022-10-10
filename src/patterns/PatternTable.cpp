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

	class DefensiveMoveFinder2
	{
			const PatternTable &table;
			Sign defender_sign;
			IsOpenFour is_open_four;
		public:
			DefensiveMoveFinder2(const PatternTable &t, Sign defenderSign) :
					table(t),
					defender_sign(defenderSign),
					is_open_four(t.getRules(), invertSign(defenderSign))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line)
			{
				const Sign attacker_sign = invertSign(defender_sign);
//				line.setCenter(attacker_sign);

				const int expanded_size = (table.getRules() == GameRules::CARO) ? 4 : 2;
				const int offset = expanded_size / 2;
				line = Pattern(line.size() + expanded_size, line.encode() << expanded_size);
				BitMask1D<uint16_t> result;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						std::cout << "defending at " << i << " : " << line.toString() << '\n';
						result[i - offset] = true;
						for (size_t j = offset; j < line.size() - offset; j++)
							if (line.get(j) == Sign::NONE)
							{
								line.set(j, attacker_sign);
								std::cout << "attacking at " << j << " : " << line.toString() << '\n';
								const bool tmp = is_open_four(line);
								line.set(j, Sign::NONE);
								if (tmp)
								{
									std::cout << "---can make a five\n";
									result[i - offset] = false;
									break;
								}
							}
						std::cout << '\n';
						line.set(i, Sign::NONE);
					}
				return result;
			}
	};

	class SearchMoveFinder
	{
			Sign attacker_sign;
			Sign defender_sign;
			IsFive is_five;
			IsOpenFour is_open_four;
			IsDoubleFour is_double_four;
			GameRules rules;
		public:
			SearchMoveFinder(const PatternTable &t, Sign defenderSign) :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					is_five(t.getRules(), attacker_sign),
					is_open_four(t.getRules(), attacker_sign),
					is_double_four(t.getRules(), attacker_sign),
					rules(t.getRules())
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line, int depth)
			{
				const int expanded_size = 0; //(rules == GameRules::CARO) ? 4 : 2;
				const int offset = expanded_size / 2;
				line = Pattern(line.size() + expanded_size, line.encode() << expanded_size); // expand the line with empty spaces at the ends (pessimistic assumption)

				const int initial_outcome = search(line, attacker_sign, depth);
				if (initial_outcome != 1) // not a win for attacker
					return BitMask1D<uint16_t>();

				BitMask1D<uint16_t> result;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						const int outcome = -search(line, attacker_sign, depth);
						if (outcome != -1) // not a loss for defender -> successful defensive move
							result[i - offset] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
			}
			bool is_a_win(Pattern line, int depth)
			{
				const int outcome = search(line, attacker_sign, depth);
				return outcome == 1;
			}
		private:
			int search(Pattern line, Sign sign, int depthRemaining)
			{
				assert(depthRemaining > 0);
				const int offset = 0;

				bool is_full = true;
				for (size_t i = offset; i < line.size() - offset; i++)
					is_full &= (line.get(i) != Sign::NONE);
				if (is_full)
					return 0;

				int outcome = -1;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, sign);
						int tmp = 0;
						if (is_five(line) or is_open_four(line) or is_double_four(line))
							tmp = 1;
						else
						{
							if (depthRemaining > 1)
								tmp = -search(line, invertSign(sign), depthRemaining - 1);
							else
								tmp = 0;
						}
						line.set(i, Sign::NONE);

						outcome = std::max(outcome, tmp);
					}

				return outcome;
			}
	};

	bool contains_pattern(const Pattern bigger, const Pattern smaller)
	{
		if (smaller.size() == 0)
			return false;
		assert(bigger.size() >= smaller.size());
		const uint32_t mask = power((size_t) 4, smaller.size()) - 1u;
		for (size_t i = 0; i < bigger.size() - smaller.size(); i++)
			if (((bigger.encode() >> (2 * i)) & mask) == smaller.encode())
				return true;
		return false;
	}

	std::vector<Pattern> get_unique(const PatternTable &t, int length, const std::vector<Pattern> &smaller)
	{
		std::vector<Pattern> result;
		SearchMoveFinder smf(t, Sign::CIRCLE);
		size_t total = 0;
		for (int i = 0; i < power(4, length); i++)
		{
			Pattern p(length, i);
			const bool is_a_win = smf.is_a_win(p, 5);
			if (is_a_win)
			{
				total++;
				bool flag = true;
				for (size_t j = 0; j < smaller.size(); j++)
					if (contains_pattern(p, smaller[j]))
					{
						flag = false;
						break;
					}
				if (flag)
					result.push_back(p);
			}
		}
		std::cout << "len" << length << " " << result.size() << "/" << total << '\n';
		return result;
	}

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

//		auto len6 = get_unique(*this, 6, { });
//		auto len7 = get_unique(*this, 7, len6);
//		auto len8 = get_unique(*this, 8, len7);
//		auto len9 = get_unique(*this, 9, len8);
//		auto len10 = get_unique(*this, 10, len9);
//		auto len11 = get_unique(*this, 11, len10);

//		size_t count = 0;
		SearchMoveFinder smf(*this, Sign::CIRCLE);
//		for (int i = 0; i < power(4, 8); i++)
////		int i = 21528;
//		{
//			Pattern p(8, i);
//			const bool is_a_win = smf.is_a_win(p);
//			if (is_a_win)
//			{
//				std::cout << p.toString() << '\n';
//				count++;
////				exit(0);
//			}
//			if (i % 1000 == 0)
//				std::cout << i << " : " << count << " forcing patterns\n";
//		}
//		std::cout << count << " forcing patterns\n";

//		double dt0 = getTime();
//		size_t count = 0;
//		for (int i = 0; i < power(4, 11); i++)
//		{
//			const Pattern p(11, i);
//			if (p.isValid())
//			{
//				const PatternType pt = getPatternData(i).forCross(); //getPatternData((i / 4) & 4194303).forCross();
//				if (PatternType::OPEN_4 <= pt and pt <= PatternType::FIVE)
//				{
//					Pattern line = Pattern(p.size() + 2, p.encode() << 2);
//					for (int j = 0; j < 16; j++)
//					{
//						line.set(0, static_cast<Sign>(j / 4));
//						line.set(10, static_cast<Sign>(j % 4));
//						const BitMask1D<uint16_t> mask = smf(line, 1);
//						if (mask.raw() != 0)
//						{
//							count++;
//							add_defensive_move_mask(mask);
//							if (pt == PatternType::NONE)
//							{
//							std::cout << "_'" << p.toString() << "'_" << "\n  ";
//								std::cout << p.toString() << "\n";
//								for (size_t j = 0; j < p.size(); j++)
//									std::cout << mask[j];
//								std::cout << '\n';
//							}
//						}
//					}
//				}
//			}
//		}
//		std::cout << (getTime() - dt0) << "s\n";
//		std::cout << "total of " << count << " defensive patterns\n";
//		std::cout << defensive_move_mask.size() << '\n';
//		exit(0);

//		const Pattern p("||OX__XXX___");
//		BitMask1D<uint16_t> mask = smf(p, 3);
//		std::cout << p.toString() << '\n';
//		for (size_t j = 0; j < p.size(); j++)
//			std::cout << mask[j];

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
		Pattern line("_XXXX_X___");
//		std::cout << line.encode() << '\n';
//		DefensiveMoveFinder2 dmf(*this, Sign::CIRCLE);
//		BitMask1D<uint16_t> res = dmf(line);
//
		std::cout << "raw feature = " << line.encode() << '\n';
		PatternEncoding data = getPatternData(line.encode());
		std::cout << line.toString() << " " << toString(data.forCross()) << " " << toString(data.forCircle()) << '\n';
		for (int i = 0; i < 11; i++)
			std::cout << data.mustBeUpdated(i);
		std::cout << " update mask\n";
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
				BitMask1D<uint16_t> mask_cross, mask_circle;
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
							mask_cross[spot_index] = true;
						if (circle_refuted.forCross() < feature.forCross())
							mask_circle[spot_index] = true;
					}
				if (feature.forCircle() == PatternType::OPEN_4)
					mask_cross = dmf_cross(base_line);
				if (feature.forCross() == PatternType::OPEN_4)
					mask_circle = dmf_circle(base_line);

				if (feature.forCircle() != PatternType::HALF_OPEN_3) // defensive moves for half open three won't be ever needed
					patterns[i].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(mask_cross));
				if (feature.forCross() != PatternType::HALF_OPEN_3)
					patterns[i].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(mask_circle));
				was_processed[i] = true;

				base_line.decode(i);
				base_line.flip();
				mask_cross.flip(feature_length);
				mask_circle.flip(feature_length);
				if (feature.forCircle() != PatternType::HALF_OPEN_3)
					patterns[base_line.encode()].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(mask_cross));
				if (feature.forCross() != PatternType::HALF_OPEN_3)
					patterns[base_line.encode()].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(mask_circle));
				was_processed[base_line.encode()] = true;
			}
		}
		assert(defensive_move_mask.size() < 128); // only 7 bits are reserved in PatternEncoding for storing index of defensive move mask
	}
	size_t PatternTable::add_defensive_move_mask(BitMask1D<uint16_t> mask)
	{
		if (std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask) == defensive_move_mask.end())
			defensive_move_mask.push_back(mask);
		return std::distance(defensive_move_mask.begin(), std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask));
	}

} /* namespace ag */

