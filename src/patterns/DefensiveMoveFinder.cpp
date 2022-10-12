/*
 * DefensiveMoveFinder.cpp
 *
 *  Created on: Oct 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/DefensiveMoveFinder.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;

	constexpr bool is_overline_allowed(GameRules rules, Sign sign) noexcept
	{
		constexpr bool caro_is_overline_allowed = false; // TODO here is the switch between Caro6 and Caro5 (true or false, respectively)
		return (rules == GameRules::FREESTYLE) or (rules == GameRules::RENJU and sign == Sign::CROSS)
				or (rules == GameRules::CARO and caro_is_overline_allowed);
	}
	constexpr bool is_blocked_allowed(GameRules rules, Sign sign) noexcept
	{
		return rules != GameRules::CARO;
	}

	uint32_t sub_pattern(uint32_t line, int start, int length) noexcept
	{
		return (line >> (2u * start)) & ((1u << (2u * length)) - 1u);
	}
	int get(uint32_t line, int idx) noexcept
	{
		return (line >> (2 * idx)) & 3;
	}
	void set(uint32_t &line, int idx, int value) noexcept
	{
		line &= (~(3u << (2u * idx)));
		line |= (value << (2u * idx));
	}
	template<int N>
	std::string print(uint32_t line)
	{
		std::string result;
		for (int i = 0; i < N; i++)
			result += text(static_cast<Sign>(get(line, i)));
		result += " : " + std::to_string(line);
		return result;
	}

	uint32_t get_index(uint32_t line, int left, int right) noexcept
	{
		const uint32_t idx_left = (line >> (2 * (left - 2))) & 15u;
		const uint32_t idx_right = (line >> (2 * right)) & 15u;
		return idx_left | (idx_right << 4);
	}

	class DefendFive
	{
			Sign attacker_sign;
			Sign defender_sign;
			bool allow_overline = false;
			bool allow_blocked = false;
			Pattern five_pattern;
		public:
			DefendFive(GameRules rules, Sign defenderSign) noexcept :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(is_overline_allowed(rules, attacker_sign)),
					allow_blocked(is_blocked_allowed(rules, attacker_sign)),
					five_pattern((attacker_sign == Sign::CROSS) ? Pattern("XXXXX") : Pattern("OOOOO"))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line, const int offset, int depth = 1) const noexcept
			{
				if (is_five(line)) // already is a five for attacker
					return BitMask1D<uint16_t>();
				if (search(line, attacker_sign, depth) == 0) // attacker does not have any way to create a five
					return BitMask1D<uint16_t>();

				BitMask1D<uint16_t> result;
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						const int outcome = search(line, attacker_sign, depth);
						if (outcome != 1) // not a loss for defender -> successful defensive move
							result[6 + offset + i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
			}

			static int getMaskLength() noexcept
			{
				return 5;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static std::array<uint32_t, 5> getMasks(Sign attackerSign) noexcept
			{
				if (attackerSign == Sign::CROSS)
					return std::array<uint32_t, 5>( { 85u, 277u, 325u, 337u, 340u }); // "XXXX_", "XXX_X", "XX_XX", "X_XXX", "_XXXX"
				else
					return std::array<uint32_t, 5>( { 170u, 554u, 650u, 674u, 680u }); // "OOOO_", "OOO_O", "OO_OO", "O_OOO", "_OOOO"
			}
		private:
//			int search(Pattern line, Sign sign) const noexcept
//			{
//				for (size_t i = 0; i < line.size(); i++)
//					if (line.get(i) == Sign::NONE)
//					{
//						line.set(i, sign);
//						if (is_five(line))
//							return 1;
//						line.set(i, Sign::NONE);
//					}
//				return 0;
//			}
			int search(Pattern line, Sign sign, int depthRemaining) const
			{
				assert(depthRemaining > 0);
				int outcome = -1;
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, sign);
						int tmp = 0;
						if (is_five(line))
							return 1;
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
			bool is_five(const Pattern line) const noexcept
			{
				const int len = five_pattern.size();
				for (size_t i = 1; i < line.size() - len; i++)
					if (line.subPattern(i, len) == five_pattern)
					{
						const Sign first = line.get(i - 1);
						const Sign last = line.get(i + 5);
						const bool win_overline = allow_overline ? true : (first != attacker_sign and last != attacker_sign);
						const bool win_blocked = allow_blocked ? true : not (first == defender_sign and last == defender_sign);
						if (win_overline and win_blocked)
							return true;
					}
				return false;
			}
	};

	class DefendOpenFour
	{
			Sign attacker_sign;
			Sign defender_sign;
			bool allow_overline = false;
			bool allow_blocked = false;
			Pattern open_four_pattern;
		public:
			DefendOpenFour(GameRules rules, Sign defenderSign) noexcept :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(is_overline_allowed(rules, attacker_sign)),
					allow_blocked(is_blocked_allowed(rules, attacker_sign)),
					open_four_pattern((attacker_sign == Sign::CROSS) ? Pattern("_XXXX_") : Pattern("_OOOO_"))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line, const int offset) const noexcept
			{
				if (search(line, attacker_sign) == 0) // attacker does not have any way to create an open four
					return BitMask1D<uint16_t>();

				BitMask1D<uint16_t> result;
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						const int outcome = search(line, attacker_sign);
						if (outcome != 1) // not a loss for defender -> successful defensive move
							result[6 + offset + i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
			}

			static int getMaskLength() noexcept
			{
				return 6;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static std::array<uint32_t, 4> getMasks(Sign attackerSign) noexcept
			{
				if (attackerSign == Sign::CROSS)
					return std::array<uint32_t, 4>( { 84u, 276u, 324u, 336u }); // "_XXX!_", "_XX!X_", "_X!XX_", "_!XXX_"
				else
					return std::array<uint32_t, 4>( { 168u, 542u, 648u, 672 }); // "_OOO!_", "_OO!O_", "_O!OO_", "_!OOO_"
			}
		private:
			int search(Pattern line, Sign sign) const noexcept
			{
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, sign);
						if (is_open_four(line))
							return 1;
						line.set(i, Sign::NONE);
					}
				return 0;
			}
			bool is_open_four(const Pattern line) const noexcept
			{
				const int len = open_four_pattern.size();
				for (size_t i = 1; i < line.size() - len; i++)
					if (line.subPattern(i, len) == open_four_pattern)
					{
						const Sign first = line.get(i - 1);
						const Sign last = line.get(i + len);
						const bool win_overline = allow_overline ? true : (first != attacker_sign and last != attacker_sign);
						const bool win_blocked = allow_blocked ? true : not (first == defender_sign and last == defender_sign);
						if (win_overline and win_blocked)
							return true;
					}
				return false;
			}
	};

	class DefendDoubleFour
	{
		public:
			static int getMaskLength(int type) noexcept
			{
				assert(type == 1 || type == 2 || type == 3);
				return 6 + type;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static std::array<uint32_t, 3> getMasksType1(Sign attackerSign) noexcept
			{
				if (attackerSign == Sign::CROSS)
					return std::array<uint32_t, 3>( { 4177u, 4369u, 4417u }); // "X_XX!_X", "X_X!X_X", "X_!XX_X"
				else
					return std::array<uint32_t, 3>( { 8354u, 8738u, 8834u }); // "O_OO!_O", "O_O!O_O", "O_!OO_O"
			}
			static std::array<uint32_t, 2> getMasksType2(Sign attackerSign) noexcept
			{
				if (attackerSign == Sign::CROSS)
					return std::array<uint32_t, 2>( { 20549u, 20741u }); // "XX_X!_XX", "XX_!X_XX"
				else
					return std::array<uint32_t, 2>( { 41098u, 41482u }); // "OO_O!_OO", "OO_!O_OO"
			}
			static uint32_t getMasksType3(Sign attackerSign) noexcept
			{
				if (attackerSign == Sign::CROSS)
					return 86037u; //  "XXX_!_XXX"
				else
					return 172074u; // "OOO_!_OOO"
			}
	};

}

namespace ag
{

	DefensiveMoveFinder::DefensiveMoveFinder(GameRules rules) :
			five_defense(5, power(4, 4)),
			open_four_defense(4, power(4, 4)),
			double_four_defense(6, power(4, 4)),
			game_rules(rules)
	{
		double t0 = getTime();
		init_five();
		init_open_four();
		init_double_four();
		double t1 = getTime();
		std::cout << (t1 - t0) << std::endl;
	}
	BitMask1D<uint16_t> DefensiveMoveFinder::get_(uint32_t pattern, Sign defenderSign, PatternType threatToDefend) const
	{
		const Sign attacker_sign = invertSign(defenderSign);
		const int center = (Pattern::length(game_rules) - 1) / 2 + 1;
		switch (threatToDefend)
		{
			case PatternType::FIVE:
			{
				const std::array<uint32_t, 5> masks = DefendFive::getMasks(attacker_sign);
				const int length = DefendFive::getMaskLength();
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = center + DefendFive::getMaskOffset() + i;
					if (sub_pattern(pattern, begin, length) == masks[i])
						return five_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
				}
				return BitMask1D<uint16_t>(); // if we got here it means that the original pattern was not a five
			}
			case PatternType::OPEN_4:
			{
				const std::array<uint32_t, 4> masks = DefendOpenFour::getMasks(attacker_sign);
				const int length = DefendOpenFour::getMaskLength();
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = center + DefendOpenFour::getMaskOffset() + i;
					if (sub_pattern(pattern, begin, length) == masks[i])
						return open_four_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
				}
				return BitMask1D<uint16_t>(); // if we got here it means that the original pattern was not an open four
			}
			case PatternType::DOUBLE_4:
			{
				{ // check type 1
					std::cout << "checking type 1\n";
					const std::array<uint32_t, 3> masks_type_1 = DefendDoubleFour::getMasksType1(attacker_sign);
					const int length = DefendDoubleFour::getMaskLength(1);
					for (size_t i = 0; i < masks_type_1.size(); i++)
					{
						const int begin = center + DefendDoubleFour::getMaskOffset() + i;
						std::cout << "testing " << print<7>(sub_pattern(pattern, begin, length)) << '\n';
						if (sub_pattern(pattern, begin, length) == masks_type_1[i])
							return double_four_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
					}
				}
				{ // check type 2
					std::cout << "checking type 2\n";
					const std::array<uint32_t, 2> masks_type_2 = DefendDoubleFour::getMasksType2(attacker_sign);
					const int length = DefendDoubleFour::getMaskLength(2);
					for (size_t i = 0; i < masks_type_2.size(); i++)
					{
						const int begin = center + DefendDoubleFour::getMaskOffset() + i;
						if (sub_pattern(pattern, begin, length) == masks_type_2[i])
							return double_four_defense.at(3 + i, get_index(pattern, begin, begin + length)).get(defenderSign);
					}
				}
				{ // check type 3
					std::cout << "checking type 3\n";
					const uint32_t mask_type_3 = DefendDoubleFour::getMasksType3(attacker_sign);
					const int length = DefendDoubleFour::getMaskLength(3);
					const int begin = center + DefendDoubleFour::getMaskOffset();
					if (sub_pattern(pattern, begin, length) == mask_type_3)
						return double_four_defense.at(5, get_index(pattern, begin, begin + length)).get(defenderSign);
				}

				return BitMask1D<uint16_t>();
			}
			case PatternType::HALF_OPEN_4:
			{
				pattern |= (static_cast<uint32_t>(attacker_sign) << (2 * center));
				const std::array<uint32_t, 5> masks = DefendFive::getMasks(attacker_sign);
				const int length = DefendFive::getMaskLength();
				for (int j = -4; j <= 4; j++) // check neighborhood for five pattern
				{
					const int begin = center + j;
					const uint32_t tmp = sub_pattern(pattern, begin, length);
					for (size_t i = 0; i < masks.size(); i++) // compare sub-pattern against five masks
						if (tmp == masks[i])
						{
							BitMask1D<uint16_t> result = five_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
							//result.shift(j + (length - 1 - i)); // shift defensive moves for five to match the actual position
							//result.add(0); // ad at center
							const int shift = j + (length - 1 - static_cast<int>(i));
							result = (shift >= 0) ? (result << shift) : (result >> (-shift));
							result[center] = 1;
							return result;
						}
				}
				return BitMask1D<uint16_t>(); // if we got here it means that the original pattern was not a five
			}
			default:
				return BitMask1D<uint16_t>();
		}
	}
	/*
	 * private
	 */
	void DefensiveMoveFinder::init_five()
	{
		const DefendFive defend_cross(game_rules, Sign::CROSS);
		const DefendFive defend_circle(game_rules, Sign::CIRCLE);

		const int length = DefendFive::getMaskLength();

		for (int i = 0; i < five_defense.rows(); i++)
		{
			const Pattern cross(length, DefendFive::getMasks(Sign::CIRCLE)[i]); // pattern to defend by cross
			const Pattern circle(length, DefendFive::getMasks(Sign::CROSS)[i]); // pattern to defend by circle
			for (int j = 0; j < 256; j++)
			{
				const uint32_t left = j & 0x0000000F;
				const uint32_t right = (j & 0x000000F0) << (2 * length);

				const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
				const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

				const int offset = i - 6;
				five_defense.at(i, j).for_cross = defend_cross(extended_cross, offset);
				five_defense.at(i, j).for_circle = defend_circle(extended_circle, offset);
			}
		}
	}
	void DefensiveMoveFinder::init_open_four()
	{
		const DefendOpenFour defend_cross(game_rules, Sign::CROSS);
		const DefendOpenFour defend_circle(game_rules, Sign::CIRCLE);
		const int length = DefendOpenFour::getMaskLength();

		for (int i = 0; i < open_four_defense.rows(); i++)
		{
			const Pattern cross(length, DefendOpenFour::getMasks(Sign::CIRCLE)[i]); // pattern to defend by cross
			const Pattern circle(length, DefendOpenFour::getMasks(Sign::CROSS)[i]); // pattern to defend by circle
			for (int j = 0; j < 256; j++)
			{
				const uint32_t left = j & 0x0000000F;
				const uint32_t right = (j & 0x000000F0) << (2 * length);

				const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
				const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

				const int offset = i - 6;
				open_four_defense.at(i, j).for_cross = defend_cross(extended_cross, offset);
				open_four_defense.at(i, j).for_circle = defend_circle(extended_circle, offset);
			}
		}
	}
	void DefensiveMoveFinder::init_double_four()
	{
		const DefendFive defend_cross(game_rules, Sign::CROSS);
		const DefendFive defend_circle(game_rules, Sign::CIRCLE);

		const int depth = 3;

		for (int i = 0; i < 3; i++)
		{
			const int length = DefendDoubleFour::getMaskLength(1);
			const Pattern cross(length, DefendDoubleFour::getMasksType1(Sign::CIRCLE)[i]); // pattern to defend by cross
			const Pattern circle(length, DefendDoubleFour::getMasksType1(Sign::CROSS)[i]); // pattern to defend by circle
			for (int j = 0; j < 256; j++)
			{
				const uint32_t left = j & 0x0000000F;
				const uint32_t right = (j & 0x000000F0) << (2 * length);

				const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
				const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

				const int offset = i - 6;
				double_four_defense.at(i, j).for_cross = defend_cross(extended_cross, offset, depth);
				double_four_defense.at(i, j).for_circle = defend_circle(extended_circle, offset, depth);
			}
		}

		for (int i = 0; i < 2; i++)
		{
			const int length = DefendDoubleFour::getMaskLength(2);
			const Pattern cross(length, DefendDoubleFour::getMasksType2(Sign::CIRCLE)[i]); // pattern to defend by cross
			const Pattern circle(length, DefendDoubleFour::getMasksType2(Sign::CROSS)[i]); // pattern to defend by circle
			for (int j = 0; j < 256; j++)
			{
				const uint32_t left = j & 0x0000000F;
				const uint32_t right = (j & 0x000000F0) << (2 * length);

				const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
				const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

				const int offset = i - 6;
				double_four_defense.at(3 + i, j).for_cross = defend_cross(extended_cross, offset, depth);
				double_four_defense.at(3 + i, j).for_circle = defend_circle(extended_circle, offset, depth);
			}
		}

		const int length = DefendDoubleFour::getMaskLength(3);
		const Pattern cross(length, DefendDoubleFour::getMasksType3(Sign::CIRCLE)); // pattern to defend by cross
		const Pattern circle(length, DefendDoubleFour::getMasksType3(Sign::CROSS)); // pattern to defend by circle
		for (int j = 0; j < 256; j++)
		{
			const uint32_t left = j & 0x0000000F;
			const uint32_t right = (j & 0x000000F0) << (2 * length);

			const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
			const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

			const int offset = 0 - 6;
			double_four_defense.at(5, j).for_cross = defend_cross(extended_cross, offset, depth);
			double_four_defense.at(5, j).for_circle = defend_circle(extended_circle, offset, depth);
		}
	}
} /* namespace ag */

