/*
 * test_defensive_moves.cpp
 *
 *  Created on: Oct 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/utils/BitMask.hpp>

#include <gtest/gtest.h>

namespace
{
	using namespace ag;
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

	class SearchMoveFinder
	{
			Sign attacker_sign;
			Sign defender_sign;
			IsFive is_five;
			IsOpenFour is_open_four;
			IsDoubleFour is_double_four;
		public:
			SearchMoveFinder(GameRules rules, Sign defenderSign) :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					is_five(rules, attacker_sign),
					is_open_four(rules, attacker_sign),
					is_double_four(rules, attacker_sign)
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line, int depth) const
			{
//				std::cout << '\n';
				const int initial_outcome = search(line, attacker_sign);
				if (initial_outcome != 1) // not a win for attacker
					return BitMask1D<uint16_t>();

				BitMask1D<uint16_t> result;
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, defender_sign);
						const int outcome = -search(line, attacker_sign);
						if (outcome != -1) // not a loss for defender -> successful defensive move
							result[i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
			}
		private:
			int search(Pattern line, Sign sign, int depthRemaining) const
			{
				assert(depthRemaining > 0);
				const int offset = 0;

				int outcome = -1;
				for (size_t i = offset; i < line.size() - offset; i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, sign);
						int tmp = 0;
						if (is_five(line) or is_open_four(line) or is_double_four(line))
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
			int search(Pattern line, Sign sign) const
			{
				for (size_t i = 0; i < line.size(); i++)
					if (line.get(i) == Sign::NONE)
					{
						line.set(i, sign);
						if (is_five(line) or is_open_four(line) or is_double_four(line))
							return 1;
						line.set(i, Sign::NONE);
					}
				return 0;
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
			DefendFive(bool allowOverline, bool allowBlocked, Sign defenderSign) noexcept :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(allowOverline),
					allow_blocked(allowBlocked),
					five_pattern((attacker_sign == Sign::CROSS) ? Pattern("XXXXX") : Pattern("OOOOO"))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line, int depth = 1) const noexcept
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
							result[i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
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
						const Sign last = line.get(i + len);
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
			DefendOpenFour(bool allowOverline, bool allowBlocked, Sign defenderSign) noexcept :
					attacker_sign(invertSign(defenderSign)),
					defender_sign(defenderSign),
					allow_overline(allowOverline),
					allow_blocked(allowBlocked),
					open_four_pattern((attacker_sign == Sign::CROSS) ? Pattern("_XXXX_") : Pattern("_OOOO_"))
			{
			}
			BitMask1D<uint16_t> operator()(Pattern line) const noexcept
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
							result[i] = 1;
						line.set(i, Sign::NONE);
					}
				return result;
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
}

namespace ag
{

	TEST(TestDefensiveMoves, FreestyleFive)
	{
		GTEST_SKIP();
		const GameRules rules = GameRules::STANDARD;
		const PatternType type = PatternType::HALF_OPEN_4;
		const int expanded_size = (rules == GameRules::FREESTYLE) ? 0 : 1;

		const SearchMoveFinder smf_cross(rules, Sign::CROSS);
		const SearchMoveFinder smf_circle(rules, Sign::CIRCLE);

		DefensiveMoveTable dmt(rules);
		//          "      C      "
//		Pattern pat("___X_X_X_X___");
//		Pattern pat("___XXX_X_____");
//		Pattern pat("_OX_XX__X____");
//		Pattern pat("____X__XX_XO_");
		Pattern pat("_X_O_O_O_OOX_");
//		Pattern pat("  _X_O_OOO_OO");
//		Pattern pat("_OX_XX_______");
//		Pattern pat("XX_OOO__O_|||");
//		Pattern pat("__XX_X__XXOO|");
//		Pattern pat("|_X_XX__X____");

		assert(pat.size() == 13);
		const uint32_t reduced = (pat.encode() >> 2) & (power(4, 11) - 1);
		const PatternType actual_type = PatternTable::get(rules).getPatternType(NormalPattern(reduced)).forCircle();
		std::cout << toString(actual_type) << '\n';

		const BitMask1D<uint16_t> tmp = dmt.getMoves(ExtendedPattern(pat.encode()), Sign::CROSS, actual_type);

		std::cout << "      C\n" << pat.toString() << '\n';
		for (size_t j = 0; j < pat.size(); j++)
			std::cout << tmp[j];
		std::cout << '\n';
//		std::string str(pat.size(), '_');
//		for (int j = 0; j < tmp.length(); j++)
//			str.at((pat.size() - 1) / 2 + tmp[j]) = '!';
//		std::cout << str << '\n';
		exit(0);

		std::vector<BitMask1D<uint16_t>> masks;
		BitMask1D<uint16_t> correct[4];
		const int max_combinations = 16;
		for (int i = 0; i < max_combinations; i++)
//		int i = 17;
		{
			// fives  "_XXXX", "X_XXX", "XX_XX", "XXX_X", "XXXX_
			// open 4 "_XXX__", "_XX_X_", "_X_XX_", "__XXX_"
			// half open four "__XXX", "_X_XX", "_XX_X", "_XXX_", "X__XX", "X_X_X", "X_XX_", "XX__X", "XX_X_", "XXX__"
			// double four "X_XXX_X", "XX_XX_XX", "XXX_X_XXX"
			/*       C
			 *  "XXX__"
			 *  "XX_X_"
			 *  "X_XX_"
			 *  "_XXX_"
			 *  "    _XXX_"
			 *  "    _XX_X"
			 *  "    _X_XX"
			 *  "    __XXX"
			 */
			Pattern p("_X__XX_XO");
			if (max_combinations == 256)
				p = Pattern(4 + p.size(), (p.encode() << 4) | (i & 15) | ((i >> 4) << (4 + 2 * p.size())));
			if (max_combinations == 16)
				p = Pattern(2 + p.size(), (p.encode() << 2) | (i & 3) | ((i >> 2) << (2 + 2 * p.size())));

			bool flag = true;
			for (int k = 0; k < 4; k++)
//			int k = 1;
			{
				const BitMask1D<uint16_t> tmp = DefendFive(1 - k % 2, 1 - k / 2, Sign::CIRCLE)(p, 5);
//				const BitMask1D<uint16_t> tmp = DefendOpenFour(1 - k % 2, 1 - k / 2, Sign::CROSS)(p);
				if (tmp != correct[k])
					flag = false;
				correct[k] = tmp;
			}

			if (flag == false)
				std::cout << '\n';
			std::cout << p.toString() << " : ";
			for (int k = 0; k < 4; k++)
			{
				for (size_t j = 0; j < p.size(); j++)
					std::cout << correct[k][j];
				std::cout << "  ";
			}
			std::cout << ":  " << i << '\n';

		}
//		std::cout << masks.size() << " unique masks\n";
//		for (size_t i = 0; i < masks.size(); i++)
//		{
//			for (size_t j = 0; j < 5 + 4; j++)
//				std::cout << masks[i][j];
//			std::cout << '\n';
//		}
		exit(0);

//		DefensiveMoveFinder dmf;

//		const int length = Pattern::length(rules) + 2 * expanded_size;
//		const int number_of_patterns = power(4, length);

//		int count = 0;
////		for (int i = 0; i < number_of_patterns; i++)
//		{
////			Pattern p(length, i);
//			Pattern p("O_XXXX_______");
//			int i = p.encode();
//			if (p.isValid())
//			{
//				const uint32_t reduced = (i >> (2 * expanded_size)) & (power(4, length - 2 * expanded_size) - 1);
//				const PatternEncoding encoding = PatternTable::get(rules).getPatternData(reduced);
//				if (encoding.forCross() == type)
//				{
////					p.setCenter(Sign::CROSS);
//					const BitMask1D<uint16_t> correct = smf_circle(p, 3);
////					const BitMask1D<uint16_t> tested = dmf.get(i, Sign::CIRCLE);
//					const BitMask1D<uint16_t> tested = PatternTable::get(rules).getDefensiveMoves(encoding, Sign::CIRCLE).raw() << expanded_size;
////					const BitMask1D<uint16_t> tested = smf_circle(Pattern(length - 2 * expanded_size, reduced), 3).raw() << expanded_size;
////					EXPECT_EQ(correct, tested);
//					if (correct != tested)
//					{
//						std::cout << p.toString() << '\n';
//						std::cout << " " << Pattern(length - 2 * expanded_size, reduced).toString() << '\n';
//						for (size_t j = 0; j < p.size(); j++)
//							std::cout << correct[j];
//						std::cout << " <- correct\n";
//						for (size_t j = 0; j < p.size(); j++)
//							std::cout << tested[j];
//						std::cout << " <- tested\n";
//						count++;
//
//					}
//				}
//
////				if (encoding.forCircle() == type)
////				{
////					const BitMask1D<uint16_t> correct = smf_cross(p, 3);
////					const BitMask1D<uint16_t> tested = dmf.get(i, Sign::CROSS);
////					EXPECT_EQ(correct, tested);
////				}
//			}
//			if (count > 30)
//				exit(0);
//		}
//		std::cout << count << '\n';
//		exit(0);
	}

} /* namespace ag */

