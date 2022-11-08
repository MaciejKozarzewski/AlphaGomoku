/*
 * DefensiveMoveTable.cpp
 *
 *  Created on: Oct 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;

	constexpr bool is_overline_allowed(GameRules rules, Sign attackerSign) noexcept
	{
		return (rules == GameRules::FREESTYLE) or (rules == GameRules::RENJU and attackerSign == Sign::CIRCLE) or (rules == GameRules::CARO6);
	}
	constexpr bool is_blocked_allowed(GameRules rules, Sign sign) noexcept
	{
		return rules != GameRules::CARO5 and rules != GameRules::CARO6;
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

	class CanBecomeFive
	{
			Sign attacker_sign;
			Sign defender_sign;
			bool allow_overline = false;
			bool allow_blocked = false;
		public:
			CanBecomeFive(GameRules rules, Sign defenderSign) :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(is_overline_allowed(rules, attacker_sign)),
					allow_blocked(is_blocked_allowed(rules, attacker_sign))
			{
			}
			bool operator()(const uint32_t line, int offset) const noexcept
			{
				const Sign first = static_cast<Sign>(get(line, offset - 1));
				const Sign last = static_cast<Sign>(get(line, offset + 5));
				if (not allow_overline and (first == attacker_sign or last == attacker_sign))
					return false; // check if any stone on the sides is in the color of an attacker
				if (not allow_blocked and (first == defender_sign and last == defender_sign))
					return false; // check if both stones on the sides are in the color of the defender

				int sum_c = 0;
				int sum_empty = 0;
				for (int i = 0; i < 5; i++)
				{
					sum_c += static_cast<int>(get(line, offset + i) == static_cast<int>(attacker_sign));
					sum_empty += static_cast<int>(get(line, offset + i) == static_cast<int>(Sign::NONE));
				}
				return (sum_c == 4) and (sum_empty == 1);
			}
	};

	class CheckSides
	{
			Sign attacker_sign;
			Sign defender_sign;
			bool allow_overline = false;
			bool allow_blocked = false;
		public:
			CheckSides(GameRules rules, Sign defenderSign) :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(is_overline_allowed(rules, attacker_sign)),
					allow_blocked(is_blocked_allowed(rules, attacker_sign))
			{
			}
			bool operator()(const uint32_t line, int offset) const noexcept
			{
				const Sign first = static_cast<Sign>(get(line, offset - 1));
				const Sign last = static_cast<Sign>(get(line, offset + 5));
				if (not allow_overline and (first == attacker_sign or last == attacker_sign))
					return false; // check if any stone on the sides is in the color of an attacker
				if (not allow_blocked and (first == defender_sign and last == defender_sign))
					return false; // check if both stones on the sides are in the color of the defender
				return true;
			}
	};

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
							result[offset + i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
			}

			static constexpr int getMaskLength() noexcept
			{
				return 5;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			/*
			 * \brief Returns an offset relative to the beginning of the extended pattern (pattern of length 13)
			 */
			static constexpr int getOffset(int i) noexcept
			{
				assert(0 <= i && i < 5);
				return 2 + i;
			}
			static const std::array<uint32_t, 5>& getMasks(Sign attackerSign) noexcept
			{
				static const std::array<uint32_t, 5> cross( { 85u, 277u, 325u, 337u, 340u }); // "XXXX_", "XXX_X", "XX_XX", "X_XXX", "_XXXX"
				static const std::array<uint32_t, 5> circle( { 170u, 554u, 650u, 674u, 680u }); // "OOOO_", "OOO_O", "OO_OO", "O_OOO", "_OOOO"
				return (attackerSign == Sign::CROSS) ? cross : circle;
			}
		private:
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
		public:
			static constexpr int getMaskLength() noexcept
			{
				return 6;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static constexpr int getOffset(int i) noexcept
			{
				assert(0 <= i && i < 4);
				return 2 + i;
			}
			static const std::array<uint32_t, 4>& getMasks(Sign attackerSign) noexcept
			{
				static const std::array<uint32_t, 4> cross( { 84u, 276u, 324u, 336u }); // "_XXX!_", "_XX!X_", "_X!XX_", "_!XXX_"
				static const std::array<uint32_t, 4> circle( { 168u, 552u, 648u, 672u }); // "_OOO!_", "_OO!O_", "_O!OO_", "_!OOO_"
				return (attackerSign == Sign::CROSS) ? cross : circle;
			}
	};

	class DefendDoubleFour
	{
		public:
			static const int getMaskLength(int i) noexcept
			{
				static const std::array<int, 6> lengths( { 7, 7, 7, 8, 8, 9 });
				return lengths[i];
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static const int getOffset(int i) noexcept
			{
				static const std::array<int, 6> offsets( { 2, 3, 4, 2, 3, 2 });
				return offsets[i];
			}
			static const std::array<uint32_t, 6>& getMasks(Sign attackerSign) noexcept
			{
				static const std::array<uint32_t, 6> cross( { 4177u, 4369u, 4417u, 20549u, 20741u, 86037u }); // "X_XX!_X", "X_X!X_X", "X_!XX_X",  "XX_X!_XX", "XX_!X_XX", "XXX_!_XXX"
				static const std::array<uint32_t, 6> circle( { 8354u, 8738u, 8834u, 41098u, 41482u, 172074u }); // "O_OO!_O", "O_O!O_O", "O_!OO_O", "OO_O!_OO", "OO_!O_OO", "OOO_!_OOO"
				return (attackerSign == Sign::CROSS) ? cross : circle;
			}
	};

	class DefendHalfOpenFour
	{
		public:
			static constexpr int getMaskLength() noexcept
			{
				return 5;
			}
			static int getMaskOffset() noexcept
			{
				return -4;
			}
			static const int getOffset(int i) noexcept
			{
				static const std::array<int, 20> offsets( { 3, 4, 5, 6, 2, 4, 5, 6, 2, 3, 5, 6, 2, 3, 4, 6, 2, 3, 4, 5 });
				return offsets[i];
			}
			static const std::array<uint32_t, 20>& getMasks(Sign attackerSign) noexcept
			{
//				"XXX!_", "XX!X_", "X!XX_", "!XXX_"
//				"XXX_!", "XX!_X", "X!X_X", "!XX_X
//				"XX_X!", "XX_!X", "X!_XX", "!X_XX"
//				"X_XX!", "X_X!X", "X_!XX", "!_XXX"
//				"_XXX!", "_XX!X", "_X!XX", "_!XXX"
				static const std::array<uint32_t, 20> cross( { 21u, 69u, 81u, 84u, 21u, 261u, 273u, 276u, 69u, 261u, 321u, 324u, 81u, 273u, 321u,
						336u, 84u, 276u, 324u, 336u });
				static const std::array<uint32_t, 20> circle( { 42u, 138u, 162u, 168u, 42u, 522u, 546u, 552u, 138u, 522u, 642u, 648u, 162u, 546u,
						642u, 672u, 168u, 552u, 648u, 672u });
				return (attackerSign == Sign::CROSS) ? cross : circle;
			}
	};

	class DefendOpenThree
	{
		public:
			static constexpr int getMaskLength() noexcept
			{
				return 6;
			}
			static std::array<int, 12> getMaskOffsets() noexcept
			{
				return std::array<int, 12>( { 3, 4, 5, 2, 4, 5, 2, 3, 5, 2, 3, 4 });
			}
			static const int getOffset(int i) noexcept
			{
				static const std::array<int, 12> offsets( { 3, 4, 5, 2, 4, 5, 2, 3, 5, 2, 3, 4 });
				return offsets[i];
			}
			static const std::array<uint32_t, 12>& getMasks(Sign attackerSign) noexcept
			{
				static const std::array<uint32_t, 12> cross( { 20u, 68u, 80u, 20u, 260u, 272u, 68u, 260u, 320u, 80u, 272u, 320u });
				static const std::array<uint32_t, 12> circle( { 40u, 136u, 160u, 40u, 520u, 544u, 136u, 520u, 640u, 160u, 544u, 640u });
				return (attackerSign == Sign::CROSS) ? cross : circle;
//				 _XX!__
//				  _X!X__
//				   _!XX__
//				_XX_!_
//				  _X!_X_
//				   _!X_X_
//				_X_X!_
//				 _X_!X_
//				   _!_XX_
//				__XX!_
//				 __X!X_
//				  __!XX_
			}
	};

}

namespace ag
{

	BitMask1D<uint16_t> getOpenThreePromotionMoves(uint32_t pattern) noexcept
	{
		// masks   "_XXX__", "_XX_X_", "_X_XX_", "__XXX_"
		// results "!___!!", "!__!_!", "!_!__!", "!!___!"
		pattern |= (static_cast<uint32_t>(Sign::CROSS) << 10); // place cross at the center
		const std::array<uint32_t, 4> masks = { 84u, 276u, 324u, 336u };
		const std::array<uint16_t, 4> results = { 49u, 41u, 37u, 35u };
		const int length = 6;
		for (int j = -5; j <= 5; j++)
		{
			const uint32_t tmp = sub_pattern(pattern, 6 + j, length);
			for (size_t i = 0; i < masks.size(); i++)
				if (tmp == masks[i])
				{
					const int shift = 6 + j;
					return (shift >= 0) ? (results[i] << shift) : (results[i] >> (-shift));
				}
		}
		return BitMask1D<uint16_t>();
	}

	DefensiveMoveTable::DefensiveMoveTable(GameRules rules) :
			five_defense(5, power(4, 4)),
			open_four_defense(4, power(4, 4)),
			double_four_defense(6, power(4, 4)),
			game_rules(rules)
	{
//		double t0 = getTime();
		init_five();
		init_open_four();
		init_double_four();
//		double t1 = getTime();
//		std::cout << (t1 - t0) << std::endl;
	}
	BitMask1D<uint16_t> DefensiveMoveTable::getMoves(uint32_t pattern, Sign defenderSign, PatternType threatToDefend) const
	{
		const Sign attacker_sign = invertSign(defenderSign);
		const int center = (Pattern::length - 1) / 2 + 1;
		switch (threatToDefend)
		{
			case PatternType::FIVE:
			{
				const std::array<uint32_t, 5> &masks = DefendFive::getMasks(attacker_sign);
				const int length = DefendFive::getMaskLength();
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = DefendFive::getOffset(i);
					if (sub_pattern(pattern, begin, length) == masks[i])
						return five_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
				}
				return BitMask1D<uint16_t>();
			}
			case PatternType::OPEN_4:
			{
				const std::array<uint32_t, 4> &masks = DefendOpenFour::getMasks(attacker_sign);
				const int length = DefendOpenFour::getMaskLength();
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = DefendOpenFour::getOffset(i);
					if (sub_pattern(pattern, begin, length) == masks[i])
						return open_four_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
				}
				return BitMask1D<uint16_t>();
			}
			case PatternType::DOUBLE_4:
			{
				const std::array<uint32_t, 6> &masks = DefendDoubleFour::getMasks(attacker_sign);
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int length = DefendDoubleFour::getMaskLength(i);
					const int begin = DefendDoubleFour::getOffset(i);
					if (sub_pattern(pattern, begin, length) == masks[i])
						return double_four_defense.at(i, get_index(pattern, begin, begin + length)).get(defenderSign);
				}
				return BitMask1D<uint16_t>();
			}
			case PatternType::HALF_OPEN_4:
			{
				const CheckSides check_sides(game_rules, defenderSign);
				const std::array<uint32_t, 20> &masks = DefendHalfOpenFour::getMasks(attacker_sign);
				const int length = DefendHalfOpenFour::getMaskLength();
				BitMask1D<uint16_t> result(1u << center);
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = DefendHalfOpenFour::getOffset(i);
					if (sub_pattern(pattern, begin, length) == masks[i]) // and check_sides(pattern, begin))
					{
						BitMask1D<uint16_t> tmp = five_defense.at(i / 4, get_index(pattern, begin, begin + length)).get(defenderSign);
						const int shift = begin - DefendFive::getOffset(i / 4); // shift is required as half open four patterns are not aligned with five ones
						tmp = (shift >= 0) ? (tmp << shift) : (tmp >> (-shift));
						result |= tmp;
						if (game_rules != GameRules::CARO5 and game_rules != GameRules::CARO6)
							return result; // only in caro it is possible to refute more than one half open four with a single move
					}
				}
				return result;
			}
			case PatternType::OPEN_3:
			{
				const std::array<uint32_t, 12> &masks = DefendOpenThree::getMasks(attacker_sign);
				const int length = DefendOpenThree::getMaskLength();
				for (size_t i = 0; i < masks.size(); i++)
				{
					const int begin = DefendOpenThree::getOffset(i);
					if (sub_pattern(pattern, begin, length) == masks[i])
					{
						BitMask1D<uint16_t> result = open_four_defense.at(i / 3, get_index(pattern, begin, begin + length)).get(defenderSign);
						const int shift = begin - DefendOpenFour::getOffset(i / 3); // shift is required as open three patterns are not aligned with open four ones
						result = (shift >= 0) ? (result << shift) : (result >> (-shift));
						result[center] = 1;
						return result;
					}
				}
				return BitMask1D<uint16_t>();
			}
			default:
				return BitMask1D<uint16_t>();
		}
	}
	const DefensiveMoveTable& DefensiveMoveTable::get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static const DefensiveMoveTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static const DefensiveMoveTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static const DefensiveMoveTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO5:
			{
				static const DefensiveMoveTable table(GameRules::CARO5);
				return table;
			}
			case GameRules::CARO6:
			{
				static const DefensiveMoveTable table(GameRules::CARO6);
				return table;
			}
			default:
				throw std::logic_error("DefensiveMoveTable::get() unknown rule " + std::to_string((int) rules));
		}
	}
	/*
	 * private
	 */
	void DefensiveMoveTable::init_five()
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

				const int offset = DefendFive::getOffset(i) - 2;
				five_defense.at(i, j).for_cross = defend_cross(extended_cross, offset);
				five_defense.at(i, j).for_circle = defend_circle(extended_circle, offset);
			}
		}
	}
	void DefensiveMoveTable::init_open_four()
	{
		const DefendFive defend_cross(game_rules, Sign::CROSS);
		const DefendFive defend_circle(game_rules, Sign::CIRCLE);
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

				const int offset = DefendOpenFour::getOffset(i) - 2;
				open_four_defense.at(i, j).for_cross = defend_cross(extended_cross, offset, 3);
				open_four_defense.at(i, j).for_circle = defend_circle(extended_circle, offset, 3);
			}
		}
	}
	void DefensiveMoveTable::init_double_four()
	{
		const DefendFive defend_cross(game_rules, Sign::CROSS);
		const DefendFive defend_circle(game_rules, Sign::CIRCLE);

		for (int i = 0; i < 6; i++)
		{
			const int length = DefendDoubleFour::getMaskLength(i);
			const Pattern cross(length, DefendDoubleFour::getMasks(Sign::CIRCLE)[i]); // pattern to defend by cross
			const Pattern circle(length, DefendDoubleFour::getMasks(Sign::CROSS)[i]); // pattern to defend by circle
			for (int j = 0; j < 256; j++)
			{
				const uint32_t left = j & 0x0000000F;
				const uint32_t right = (j & 0x000000F0) << (2 * length);

				const Pattern extended_cross(length + 4, left | (cross.encode() << 4) | right);
				const Pattern extended_circle(length + 4, left | (circle.encode() << 4) | right);

				const int offset = DefendDoubleFour::getOffset(i) - 2;
				double_four_defense.at(i, j).for_cross = defend_cross(extended_cross, offset, 3);
				double_four_defense.at(i, j).for_circle = defend_circle(extended_circle, offset, 3);
			}
		}
	}

} /* namespace ag */

