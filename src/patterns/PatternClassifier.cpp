/*
 * PatternClassifier.cpp
 *
 *  Created on: Jul 17, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternClassifier.hpp>

#include <algorithm>

namespace ag
{

	MatchingRule::MatchingRule(const std::string &str)
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
	bool MatchingRule::isMatching(const Pattern &f) const noexcept
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
	int MatchingRule::findMatch(const Pattern &f) const noexcept
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
	std::string MatchingRule::toString() const
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

	PatternClassifier::PatternClassifier(GameRules rule, Sign sign) :
			m_rule(rule),
			m_sign(sign)
	{
		assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
	}
	Sign PatternClassifier::getSign() const noexcept
	{
		return m_sign;
	}
	bool PatternClassifier::operator()(const Pattern &f) const noexcept
	{
		for (size_t i = 0; i < m_matching_rules.size(); i++)
			if (m_matching_rules[i].findMatch(f) != -1)
				return true;
		return false;
	}
	void PatternClassifier::print() const
	{
		for (size_t i = 0; i < m_matching_rules.size(); i++)
			std::cout << m_matching_rules[i].toString() << '\n';
	}
	void PatternClassifier::addPattern(const std::string &str)
	{
		m_matching_rules.emplace_back(str);
	}
	void PatternClassifier::addPatterns(const std::vector<std::string> &str)
	{
		for (size_t i = 0; i < str.size(); i++)
			addPattern(str[i]);
	}
	void PatternClassifier::modifyPatternsAND(const std::string &prefix, const std::string &postfix)
	{
		for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
			*iter = MatchingRule(prefix + iter->toString() + postfix);
	}
	void PatternClassifier::modifyPatternsOR(const std::string &prefix, const std::string &postfix)
	{
		std::vector<MatchingRule> result;
		for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
		{
			result.push_back(MatchingRule(prefix + iter->toString() + "[any]"));
			result.push_back(MatchingRule("[any]" + iter->toString() + postfix));
		}
		m_matching_rules = result;
	}
	void PatternClassifier::modifyPatternsOR(const std::string &prefix, const std::string &common, const std::string &postfix)
	{
		std::vector<MatchingRule> result;
		for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
		{
			result.push_back(MatchingRule(prefix + iter->toString() + common));
			result.push_back(MatchingRule(common + iter->toString() + postfix));
		}
		m_matching_rules = result;
	}

	IsOverline::IsOverline(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
			addPattern("XXXXXX");
		else
			addPattern("OOOOOO");
	}
	IsFive::IsFive(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPattern("XXXXX");
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not X]", "[_|]");
		}
		else
		{
			addPattern("OOOOO");
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not O]", "[_|]");
		}
	}
	IsOpenFour::IsOpenFour(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPattern("_XXXX_");
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
		}
		else
		{
			addPattern("_OOOO_");
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
		}
		if (rule == GameRules::CARO)
			modifyPatternsAND("[_|]", "[_|]");
	}
	IsDoubleFour::IsDoubleFour(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPatterns( { "X_XXX_X", "XX_XX_XX", "XXX_X_XXX" });
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
		}
		else
		{
			addPatterns( { "O_OOO_O", "OO_OO_OO", "OOO_O_OOO" });
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
		}
		if (rule == GameRules::CARO)
			modifyPatternsAND("[_|]", "[_|]");
	}
	IsHalfOpenFour::IsHalfOpenFour(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPatterns( { "_XXXX", "X_XXX", "XX_XX", "XXX_X", "XXXX_" });
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not X]", "[_|]");
		}
		else
		{
			addPatterns( { "_OOOO", "O_OOO", "OO_OO", "OOO_O", "OOOO_" });
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not O]", "[_|]");
		}
	}
	IsOpenThree::IsOpenThree(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPatterns( { "_XXX__", "_XX_X_", "_X_XX_", "__XXX_" });
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
		}
		else
		{
			addPatterns( { "_OOO__", "_OO_O_", "_O_OO_", "__OOO_" });
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
		}
		if (rule == GameRules::CARO)
			modifyPatternsAND("[_|]", "[_|]");
	}
	IsHalfOpenThree::IsHalfOpenThree(GameRules rule, Sign sign) :
			PatternClassifier(rule, sign)
	{
		if (sign == Sign::CROSS) // black
		{
			addPatterns( { "__XXX", "_X_XX", "_XX_X", "_XXX_", "X__XX", "X_X_X", "X_XX_", "XX__X", "XX_X_", "XXX__" });
			if (rule == GameRules::STANDARD or rule == GameRules::RENJU)
				modifyPatternsAND("[not X]", "[not X]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not X]", "[_|]");
		}
		else
		{
			addPatterns( { "__OOO", "_O_OO", "_OO_O", "_OOO_", "O__OO", "O_O_O", "O_OO_", "OO__O", "OO_O_", "OOO__" });
			if (rule == GameRules::STANDARD)
				modifyPatternsAND("[not O]", "[not O]");
			if (rule == GameRules::CARO)
				modifyPatternsOR("[_|]", "[not O]", "[_|]");
		}
	}

} /* namespace ag */

