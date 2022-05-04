/*
 * PatternTable.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/solver/PatternTable.hpp>
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

	constexpr Sign signFromText(char c) noexcept
	{
		switch (c)
		{
			case '_':
				return Sign::NONE;
			case 'X':
				return Sign::CROSS;
			case 'O':
				return Sign::CIRCLE;
			default:
				return Sign::ILLEGAL;
		}
	}

	template<typename T>
	constexpr T power(T base, T exponent) noexcept
	{
		T result = 1;
		for (T i = 0; i < exponent; i++)
			result *= base;
		return result;
	}

	constexpr uint32_t reverse_bits(uint32_t x) noexcept
	{
		x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
		x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
		x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
		x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
		x = (x >> 16) | (x << 16);
		return x;
	}

	class Pattern
	{
		private:
			uint32_t m_feature = 0;
			uint32_t m_length = 0;
		public:
			static constexpr int length(GameRules rules) noexcept
			{
				switch (rules)
				{
					case GameRules::FREESTYLE:
						return 9;
					case GameRules::STANDARD:
					case GameRules::RENJU:
					case GameRules::CARO:
						return 11;
					default:
						return 0;
				}
			}

			Pattern() = default;
			Pattern(int len) :
					m_feature(0),
					m_length(len)
			{
			}
			Pattern(int len, uint32_t feature) :
					Pattern(len)
			{
				m_feature = feature;
			}
			Pattern(const std::string &str) :
					Pattern(str.size())
			{
				for (uint32_t i = 0; i < m_length; i++)
					set(i, signFromText(str[i]));
			}
			bool isValid() const noexcept
			{
				if (getCenter() != Sign::NONE)
					return false;
				for (uint32_t i = 0; i < m_length / 2; i++)
					if (get(i) != Sign::ILLEGAL and get(i + 1) == Sign::ILLEGAL)
						return false;
				for (uint32_t i = 1 + m_length / 2; i < m_length; i++)
					if (get(i - 1) == Sign::ILLEGAL and get(i) != Sign::ILLEGAL)
						return false;
				return true;
			}
			void invert() noexcept
			{
				for (uint32_t i = 0; i < m_length; i++)
					set(i, invertSign(get(i)));
			}
			void flip() noexcept
			{
				m_feature = ((m_feature >> 2) & 0x33333333) | ((m_feature & 0x33333333) << 2);
				m_feature = ((m_feature >> 4) & 0x0F0F0F0F) | ((m_feature & 0x0F0F0F0F) << 4);
				m_feature = ((m_feature >> 8) & 0x00FF00FF) | ((m_feature & 0x00FF00FF) << 8);
				m_feature = (m_feature >> 16) | (m_feature << 16);
				m_feature >>= (32u - 2 * m_length);
			}
			Sign get(uint32_t index) const noexcept
			{
				assert(index < m_length);
				return static_cast<Sign>((m_feature >> (2 * index)) & 3);
			}
			void set(uint32_t index, Sign sign) noexcept
			{
				assert(index < m_length);
				m_feature &= (~(3 << (2 * index)));
				m_feature |= (static_cast<uint32_t>(sign) << (2 * index));
			}
			Sign getCenter() const noexcept
			{
				return get(m_length / 2);
			}
			void setCenter(Sign sign) noexcept
			{
				set(m_length / 2, sign);
			}
			void decode(uint32_t feature) noexcept
			{
				m_feature = feature;
			}
			uint32_t encode() const noexcept
			{
				return m_feature;
			}
			std::string toString() const
			{
				std::string result;
				for (size_t i = 0; i < m_length; i++)
					result += text(get(i));
				return result;
			}
			size_t size() const noexcept
			{
				return m_length;
			}
			void shiftLeft(size_t n) noexcept
			{
				m_feature >>= (2 * n); // this is intentionally inverted, as the features are read from left to right
			}
			void shiftRight(size_t n) noexcept
			{
				const uint32_t mask = (1 << (2 * m_length)) - 1;
				m_feature = (m_feature << (2 * n)) & mask; // this is intentionally inverted, as the features are read from left to right
			}
			void mergeWith(const Pattern &other) noexcept
			{
				assert(this->size() == other.size());
				m_feature |= other.m_feature;
			}
	};

	class MatchingRule
	{
			std::vector<std::array<bool, 4>> m_allowed_values;
		public:
			MatchingRule(const std::string &str)
			{
				size_t i = 0;
				for (; i < str.size(); i++)
				{
					std::array<bool, 4> tmp = { false, false, false, false };
					switch (str[i])
					{
						case '_':
						case 'X':
						case 'O':
						case '|':
							tmp[static_cast<int>(signFromText(str[i]))] = true;
							break;
						case '[':
							i += 1; // skip over '['
							if (str.substr(i, 4) == "not ")
							{
								tmp.fill(true);
								tmp[static_cast<int>(signFromText(str[i + 4]))] = false;
								i += 5;
							}
							else
							{
								if (str.substr(i, 3) == "any")
								{
									tmp.fill(true);
									i += 3;
								}
								else
								{
									while (str[i] != ']' and i < str.size())
									{
										assert(str[i] == '_' || str[i] == 'X' || str[i] == 'O' || str[i] == '|');
										tmp[static_cast<int>(signFromText(str[i]))] = true;
										i++;
									}
								}
							}
							break;
						default:
							throw std::logic_error("Incorrect rule '" + str + "'");
					}
					m_allowed_values.push_back(tmp);
				}
			}
			bool isMatching(const Pattern &f) const noexcept
			{
				if (f.size() < m_allowed_values.size())
					return false;
				for (size_t i = 0; i <= f.size() - m_allowed_values.size(); i++)
				{
					bool is_a_match = true;
					for (size_t j = 0; j < m_allowed_values.size(); j++)
						is_a_match &= m_allowed_values[j][static_cast<int>(f.get(i + j))];
					if (is_a_match)
						return true;
				}
				return false;
			}
			int findMatch(const Pattern &f) const noexcept
			{
				if (f.size() < m_allowed_values.size())
					return -1;
				for (size_t i = 0; i <= f.size() - m_allowed_values.size(); i++)
				{
					bool is_a_match = true;
					for (size_t j = 0; j < m_allowed_values.size(); j++)
						is_a_match &= m_allowed_values[j][static_cast<int>(f.get(i + j))];
					if (is_a_match)
						return i;
				}
				return -1;
			}
			std::string toString() const
			{
				std::string result;
				for (auto iter = m_allowed_values.begin(); iter < m_allowed_values.end(); iter++)
				{
					const int tmp = std::count(iter->begin(), iter->end(), true);
					if (tmp == 0)
						result += "[]";
					else
					{
						if (tmp != 1)
							result += "[";
						switch (tmp)
						{
							case 4:
								result += "any";
								break;
							case 3:
								result += "not ";
								for (size_t i = 0; i < iter->size(); i++)
									if (not iter->at(i))
										result += text(static_cast<Sign>(i));
								break;
							default:
								for (size_t i = 0; i < iter->size(); i++)
									if (iter->at(i))
										result += text(static_cast<Sign>(i));
						}
						if (tmp != 1)
							result += "]";
					}
				}
				return result;
			}
	};

	class FeatureClassifier
	{
			std::vector<MatchingRule> m_matching_rules;
			std::vector<BitMask<uint16_t>> m_defensive_moves;
			GameRules m_rule;
			Sign m_sign;
		public:
			FeatureClassifier(GameRules rule, Sign sign) :
					m_rule(rule),
					m_sign(sign)
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
			}
			Sign getSign() const noexcept
			{
				return m_sign;
			}
			std::pair<bool, BitMask<uint16_t>> operator()(const Pattern &f) const noexcept
			{
				assert(m_matching_rules.size() == m_defensive_moves.size());
				for (size_t i = 0; i < m_matching_rules.size(); i++)
				{
					int tmp = m_matching_rules[i].findMatch(f);
					if (tmp != -1)
						return std::pair<bool, BitMask<uint16_t>>(true, m_defensive_moves[i] << tmp);
				}
				return std::pair<bool, BitMask<uint16_t>>(false, 0);
			}
			void addPattern(const std::string &str)
			{
				m_matching_rules.emplace_back(str);
			}
			void addPatterns(const std::vector<std::string> &str)
			{
				for (size_t i = 0; i < str.size(); i++)
					addPattern(str[i]);
			}
			void modifyPatterns(const std::string &prefix, const std::string &postfix)
			{
				for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
					*iter = MatchingRule(prefix + iter->toString() + postfix);
			}
			void addDefensiveMove(const std::string &str)
			{
				m_defensive_moves.emplace_back(str);
			}
			void addDefensiveMoves(const std::vector<std::string> &str)
			{
				for (size_t i = 0; i < str.size(); i++)
					addDefensiveMove(str[i]);
			}
			void modifyDefensiveMoves(int prefix, int postfix)
			{
				for (auto iter = m_defensive_moves.begin(); iter < m_defensive_moves.end(); iter++)
				{
					uint16_t tmp = (prefix & 1) | (iter->raw() << 1) | ((postfix & 1) << Pattern::length(m_rule));
					*iter = BitMask<uint16_t>(tmp);
				}
			}
	};

	class IsOverline: public FeatureClassifier
	{
		public:
			IsOverline(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
					addPattern("XXXXXX");
				else
					addPattern("OOOOOO");
				addDefensiveMove("");
			}
	};
	class IsFive: public FeatureClassifier
	{
		public:
			IsFive(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPattern("XXXXX");
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPattern("OOOOO");
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}
				addDefensiveMove("");
			}
	};
	class IsOpenFour: public FeatureClassifier
	{
		public:
			IsOpenFour(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPattern("_XXXX_");
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPattern("_OOOO_");
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}

				addDefensiveMove("100001");
				if (rule == GameRules::STANDARD or (rule == GameRules::RENJU and sign == Sign::CROSS))
					modifyDefensiveMoves(0, 0);
				if (rule == GameRules::CARO)
					modifyDefensiveMoves(1, 1);
//				switch (rule)
//				{
//					case GameRules::FREESTYLE:
//						addDefensiveMove("100001");
//						break;
//					case GameRules::STANDARD:
//						addDefensiveMove("01000010");
//						break;
//					case GameRules::RENJU:
//						if (sign == Sign::CROSS) // black
//							addDefensiveMove("01000010");
//						else
//							addDefensiveMove("100001");
//						break;
//					case GameRules::CARO:
//						addDefensiveMove("11000011");
//						break;
//				}
			}
	};
	class IsDoubleFour: public FeatureClassifier
	{
		public:
			IsDoubleFour(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPatterns( { "X_XXX_X", "XX_XX_XX", "XXX_X_XXX" });
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPatterns( { "O_OOO_O", "OO_OO_OO", "OOO_O_OOO" });
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}

				addDefensiveMoves( { "0100010", "00100100", "000101000" });
				if (rule == GameRules::STANDARD or (rule == GameRules::RENJU and sign == Sign::CROSS))
					modifyDefensiveMoves(0, 0);
				if (rule == GameRules::CARO)
					modifyDefensiveMoves(1, 1);
//				switch (rule)
//				{
//					case GameRules::FREESTYLE:
//						addDefensiveMoves( { "0100010", "00100100", "000101000" });
//						break;
//					case GameRules::STANDARD:
//						addDefensiveMoves( { "001000100", "0001001000", "00001010000" });
//						break;
//					case GameRules::RENJU:
//						if (sign == Sign::CROSS) // black
//							addDefensiveMoves( { "001000100", "0001001000", "00001010000" });
//						else
//							addDefensiveMoves( { "0100010", "00100100", "000101000" });
//						break;
//					case GameRules::CARO:
//						addDefensiveMoves( { "101000101", "1001001001", "10001010001" });
//						break;
//				}
			}
	};
	class IsHalfOpenFour: public FeatureClassifier
	{
		public:
			IsHalfOpenFour(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPatterns( { "_XXXX", "X_XXX", "XX_XX", "XXX_X", "XXXX_" });
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPatterns( { "_OOOO", "O_OOO", "OO_OO", "OOO_O", "OOOO_" });
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}

				addDefensiveMoves( { "10000", "01000", "00100", "00010", "00001" });
				if (rule == GameRules::STANDARD or (rule == GameRules::RENJU and sign == Sign::CROSS))
					modifyDefensiveMoves(0, 0);
				if (rule == GameRules::CARO)
					modifyDefensiveMoves(1, 1);
//				switch (rule)
//				{
//					case GameRules::FREESTYLE:
//						addDefensiveMoves( { "10000", "01000", "00100", "00010", "00001" });
//						break;
//					case GameRules::STANDARD:
//						addDefensiveMoves( { "0100000", "0010000", "0001000", "0000100", "0000010" });
//						break;
//					case GameRules::RENJU:
//						if (sign == Sign::CROSS) // black
//							addDefensiveMoves( { "0100000", "0010000", "0001000", "0000100", "0000010" });
//						else
//							addDefensiveMoves( { "10000", "01000", "00100", "00010", "00001" });
//						break;
//					case GameRules::CARO:
//						addDefensiveMoves( { "1100001", "1010001", "1001001", "1000101", "1000011" });
//						break;
//				}
			}
	};
	class IsOpenThree: public FeatureClassifier
	{
		public:
			IsOpenThree(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPatterns( { "_XXX__", "_XX_X_", "_X_XX_", "__XXX_" });
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPatterns( { "_OOO__", "_OO_O_", "_O_OO_", "__OOO_" });
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}
				addDefensiveMoves( { "", "", "", "" });
			}
	};
	class IsHalfOpenThree: public FeatureClassifier
	{
		public:
			IsHalfOpenThree(GameRules rule, Sign sign) :
					FeatureClassifier(rule, sign)
			{
				if (sign == Sign::CROSS) // black
				{
					addPatterns( { "__XXX", "_X_XX", "_XX_X", "_XXX_", "X__XX", "X_X_X", "X_XX_", "XX__X", "XX_X_", "XXX__" });
					if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
						modifyPatterns("[not X]", "[not X]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not O]", "[not O]");
				}
				else
				{
					addPatterns( { "__OOO", "_O_OO", "_OO_O", "_OOO_", "O__OO", "O_O_O", "O_OO_", "OO__O", "OO_O_", "OOO__" });
					if (rule == GameRules::STANDARD)
						modifyPatterns("[not O]", "[not O]");
					if (rule == GameRules::CARO)
						modifyPatterns("[not X]", "[not X]");
				}
				addDefensiveMoves( { "", "", "", "", "", "", "", "", "", "" });
			}
	};

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
			std::pair<PatternType, BitMask<uint16_t>> operator()(const Pattern &f) const noexcept
			{
				std::pair<bool, BitMask<uint16_t>> tmp;
				tmp = is_five(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::FIVE, tmp.second);
				tmp = is_overline(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::OVERLINE, tmp.second);
				tmp = is_open_four(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::OPEN_4, tmp.second);
				tmp = is_double_four(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::DOUBLE_4, tmp.second);
				tmp = is_half_open_four(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::HALF_OPEN_4, tmp.second);
				tmp = is_open_three(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::OPEN_3, tmp.second);
				tmp = is_half_open_three(f);
				if (tmp.first)
					return std::pair<PatternType, BitMask<uint16_t>>(PatternType::HALF_OPEN_3, tmp.second);
				return std::pair<PatternType, BitMask<uint16_t>>(PatternType::NONE, BitMask<uint16_t>());
			}
	};

	class DefensiveMoveFinder
	{
			const PatternTable &table;
			IsFive is_five;
			int check_outcome(Pattern line, Sign signToMove, int depth)
			{
				std::cout << depth << " : " << line.toString() << " " << signToMove << '\n';

				if (is_five(line).first)
				{
					std::cout << depth << " : " << line.toString() << " " << signToMove << " win found\n";
					return 1; // win found
				}

				bool all_losing = true;
				bool at_least_one = false;
				for (size_t i = 1; i < line.size() - 1; i++)
					if (line.get(i) == Sign::NONE)
					{
						at_least_one = true;
						line.set(i, signToMove);
						int tmp = (depth == 7) ? 0 : check_outcome(line, invertSign(signToMove), depth + 1);
//						std::cout << "tmp = " << tmp << '\n';
						line.set(i, Sign::NONE);

						all_losing &= (tmp == -1);
						if (tmp == 1)
							return -1;
					}
				if (all_losing and at_least_one)
				{
					std::cout << depth << " : " << line.toString() << " " << signToMove << " proven loss\n";
					return 1;
				}
				else
				{
					std::cout << depth << " : " << line.toString() << " " << signToMove << " no solution\n";
					return 0;
				}
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
				std::cout << line.toString() << ":\n";
				int tmp = check_outcome(Pattern("__OOOO_X||||_"), Sign::CROSS, 0);
				std::cout << tmp << '\n';
				BitMask<uint16_t> result;
//				const Sign defender_sign = invertSign(is_five.getSign());
//				for (size_t i = 1; i < line.size() - 1; i++)
//					if (line.get(i) == Sign::NONE)
//					{
//						line.set(i, defender_sign);
//						int tmp = check_outcome(line, invertSign(defender_sign), 0);
//						if (tmp != -1)
//							result.set(i - 1, true);
//						line.set(i, Sign::NONE);
//					}
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
//		init_update_mask();
		double t2 = getTime();
//		init_defensive_moves();
		double t3 = getTime();

		std::cout << (t1 - t0) << " " << (t2 - t1) << " " << (t3 - t2) << std::endl;
		std::cout << defensive_move_mask.size() << '\n';

		std::cout << "five       : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::FIVE;}) << '\n';
		std::cout << "double 4   : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::DOUBLE_4;}) << '\n';
		std::cout << "open 4     : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::OPEN_4;}) << '\n';
		std::cout << "half open 4: " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::HALF_OPEN_4;}) << '\n';
		std::cout << "open 3     : " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::OPEN_3;}) << '\n';
		std::cout << "half open 3: " << std::count_if(patterns.begin(), patterns.end(), [](const PatternEncoding &enc)
		{	return enc.forCross() == PatternType::HALF_OPEN_3;}) << '\n' << '\n';

		Pattern line("__OOOO_____");
		DefensiveMoveFinder dmf(*this, Sign::CROSS);
		BitMask<uint16_t> res = dmf(line);

		std::cout << "raw feature = " << line.encode() << '\n';
		PatternEncoding data = getPatternData(line.encode());
		std::cout << line.toString() << " " << toString(data.forCross()) << " " << toString(data.forCircle()) << '\n';
		for (int i = 0; i < 11; i++)
			std::cout << data.mustBeUpdated(i);
		std::cout << '\n';
		for (int i = 0; i < 11; i++)
			std::cout << getDefensiveMoves(data, Sign::CROSS).get(i);
		std::cout << " defensive moves for X\n";
		for (int i = 0; i < 11; i++)
			std::cout << getDefensiveMoves(data, Sign::CIRCLE).get(i);
		std::cout << " defensive moves for O\n";

		for (int i = 0; i < 11; i++)
			std::cout << res.get(i);
		std::cout << " defensive moves for X v2\n";
		exit(0);
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
//		for (size_t i = 672; i <= 672; i++)
//			if (was_processed[i] == false)
		{
			line.decode(i);
			if (line.isValid())
			{
				line.setCenter(Sign::CROSS);
				std::pair<PatternType, BitMask<uint16_t>> cross = for_cross(line);
				line.setCenter(Sign::CIRCLE);
				std::pair<PatternType, BitMask<uint16_t>> circle = for_circle(line);
				line.setCenter(Sign::NONE);

//					cross.second.set(line.size() / 2, true); // center is always the defensive move
//					circle.second.set(line.size() / 2, true); // center is always the defensive move
//					for (size_t j = 0; j < line.size(); j++)
//						if (line.get(j) != Sign::NONE)
//						{
//							cross.second.set(j, false);
//							circle.second.set(j, false);
//						}

				patterns[i] = PatternEncoding(cross.first, circle.first);
//					was_processed[i] = true;
//					line.flip(); // the features are symmetrical so we can update both at the same time
//					patterns[line.encode()] = PatternEncoding(cross.first, circle.first);
//					was_processed[line.encode()] = true;

//					BitMask<uint16_t> cross_inverted(reverse_bits(cross.second.raw()) >> (16 - line.size()));
				patterns[i].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(circle.second));
//					patterns[line.encode()].setDefensiveMoves(Sign::CROSS, add_defensive_move_mask(cross_inverted));

//					BitMask<uint16_t> circle_inverted(reverse_bits(circle.second.raw()) >> (16 - line.size()));
				patterns[i].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(cross.second));
//					patterns[line.encode()].setDefensiveMoves(Sign::CIRCLE, add_defensive_move_mask(circle_inverted));
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
					if (spot_index != side_length) // omitting central spot as it always must be updated anyway
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
		defensive_move_mask.reserve(32);
		const int feature_length = Pattern::length(game_rules);
		Pattern base_line(feature_length);
		Pattern secondary_line(feature_length);

		for (size_t i = 0; i < patterns.size(); i++)
		{
			BitMask<uint16_t> mask_cross, mask_circle;
			base_line.decode(i);
			if (base_line.isValid())
			{
				PatternEncoding feature = getPatternData(i);
				for (int spot_index = 0; spot_index < feature_length; spot_index++)
					if (base_line.get(spot_index) == Sign::NONE)
					{
						base_line.set(spot_index, Sign::CROSS);
						const PatternEncoding cross_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::CIRCLE);
						const PatternEncoding circle_refuted = getPatternData(base_line.encode());
						base_line.set(spot_index, Sign::NONE);

//						if (feature.forCircle() != PatternType::NONE and cross_refuted.forCircle() == PatternType::NONE)
//							mask_cross.set(spot_index, true);
//						if (feature.forCross() != PatternType::NONE and circle_refuted.forCross() == PatternType::NONE)
//							mask_circle.set(spot_index, true);
						if (feature.forCircle() != cross_refuted.forCircle())
							mask_cross.set(spot_index, true);
						if (feature.forCross() != circle_refuted.forCross())
							mask_circle.set(spot_index, true);
					}

				if (std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask_cross) == defensive_move_mask.end())
					defensive_move_mask.push_back(mask_cross);
				uint32_t idx_cross = std::distance(defensive_move_mask.begin(),
						std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask_cross));
				patterns[i].setDefensiveMoves(Sign::CROSS, idx_cross);

				if (std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask_circle) == defensive_move_mask.end())
					defensive_move_mask.push_back(mask_circle);
				uint32_t idx_circle = std::distance(defensive_move_mask.begin(),
						std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask_circle));
				patterns[i].setDefensiveMoves(Sign::CIRCLE, idx_circle);
			}
		}

		assert(defensive_move_mask.size() < 128); // only 7 bits are reserved in PatternEncoding for storing index of defensive move mask

//		std::cout << toString(game_rules) << '\n';
//		for (size_t i = 0; i < defensive_move_mask.size(); i++)
//		{
//			std::cout << "\"";
//			for (int k = 0; k < feature_length; k++)
//				std::cout << defensive_move_mask[i].get(k);
//			std::cout << "\" " << defensive_move_mask[i].raw() << '\n';
//		}
//		std::cout << std::endl;
	}
	size_t PatternTable::add_defensive_move_mask(BitMask<uint16_t> mask)
	{
		if (std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask) == defensive_move_mask.end())
			defensive_move_mask.push_back(mask);
		return std::distance(defensive_move_mask.begin(), std::find(defensive_move_mask.begin(), defensive_move_mask.end(), mask));
	}

} /* namespace ag */

