/*
 * PatternClassifier.hpp
 *
 *  Created on: Jul 17, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_PATTERNCLASSIFIER_HPP_
#define ALPHAGOMOKU_SOLVER_PATTERNCLASSIFIER_HPP_

#include <alphagomoku/patterns/Pattern.hpp>

namespace ag
{

	class MatchingRule
	{
			std::vector<std::array<bool, 4>> m_allowed_values;
		public:
			MatchingRule(const std::string &str);
			bool isMatching(const Pattern &f) const noexcept;
			int findMatch(const Pattern &f) const noexcept;
			std::string toString() const;
	};

	class PatternClassifier
	{
			std::vector<MatchingRule> m_matching_rules;
			GameRules m_rule;
			Sign m_sign;
		public:
			PatternClassifier(GameRules rule, Sign sign);
			Sign getSign() const noexcept;
			bool operator()(const Pattern &f) const noexcept;
			void addPattern(const std::string &str);
			void addPatterns(const std::vector<std::string> &str);
			void modifyPatternsAND(const std::string &prefix, const std::string &postfix);
			void modifyPatternsOR(const std::string &prefix, const std::string &postfix);
			void modifyPatternsOR(const std::string &prefix, const std::string &common, const std::string &postfix);
	};

	class IsOverline: public PatternClassifier
	{
		public:
			IsOverline(GameRules rule, Sign sign);
	};
	class IsFive: public PatternClassifier
	{
		public:
			IsFive(GameRules rule, Sign sign);
	};
	class IsOpenFour: public PatternClassifier
	{
		public:
			IsOpenFour(GameRules rule, Sign sign);
	};
	class IsDoubleFour: public PatternClassifier
	{
		public:
			IsDoubleFour(GameRules rule, Sign sign);
	};
	class IsHalfOpenFour: public PatternClassifier
	{
		public:
			IsHalfOpenFour(GameRules rule, Sign sign);
	};
	class IsOpenThree: public PatternClassifier
	{
		public:
			IsOpenThree(GameRules rule, Sign sign);
	};
	class IsHalfOpenThree: public PatternClassifier
	{
		public:
			IsHalfOpenThree(GameRules rule, Sign sign);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_PATTERNCLASSIFIER_HPP_ */
