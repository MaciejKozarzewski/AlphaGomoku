/*
 * FeatureTable_v3.cpp
 *
 *  Created on: Apr 9, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureTable_v3.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <cassert>
#include <algorithm>
#include <vector>
#include <array>
#include <cstring>

namespace
{
	using namespace ag;

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

	class Feature
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

			Feature() = default;
			Feature(int len) :
					m_feature(0),
					m_length(len)
			{
			}
			Feature(int len, uint32_t feature) :
					Feature(len)
			{
				m_feature = feature;
			}
			Feature(const std::string &str) :
					Feature(str.size())
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
			Sign get(int index) const noexcept
			{
				assert(index >= 0 && index < m_length);
				return static_cast<Sign>((m_feature >> (2 * index)) & 3);
			}
			void set(int index, Sign sign) noexcept
			{
				assert(index >= 0 && index < m_length);
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
			void mergeWith(const Feature &other) noexcept
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
			bool isMatching(const Feature &f) const noexcept
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
			GameRules m_rule;
			Sign m_sign;
		public:
			FeatureClassifier(GameRules rule, Sign sign) :
					m_rule(rule),
					m_sign(sign)
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
			}
			bool operator()(const Feature &f) const noexcept
			{
				for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
					if (iter->isMatching(f) == true)
						return true;
				return false;
			}
			void addPattern(const std::string &str)
			{
				m_matching_rules.emplace_back(str);
			}
			void addPatterns(const std::vector<std::string> &str)
			{
				for (size_t i = 0; i < str.size(); i++)
					m_matching_rules.emplace_back(str[i]);
			}
			void modifyPatterns(const std::string &prefix, const std::string &postfix)
			{
				for (auto iter = m_matching_rules.begin(); iter < m_matching_rules.end(); iter++)
					*iter = MatchingRule(prefix + iter->toString() + postfix);
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
		public:
			ThreatClassifier(GameRules rules, Sign sign) :
					is_overline(rules, sign),
					is_five(rules, sign),
					is_open_four(rules, sign),
					is_double_four(rules, sign),
					is_half_open_four(rules, sign),
					is_open_three(rules, sign)
			{
			}
			FeatureType operator()(const Feature &f) const noexcept
			{
				if (is_five(f))
					return FeatureType::FIVE;
				if (is_overline(f))
					return FeatureType::OVERLINE;
				if (is_open_four(f))
					return FeatureType::OPEN_4;
				if (is_double_four(f))
					return FeatureType::DOUBLE_4;
				if (is_half_open_four(f))
					return FeatureType::HALF_OPEN_4;
				if (is_open_three(f))
					return FeatureType::OPEN_3;
				return FeatureType::NONE;
			}
	};
}

namespace ag
{

	FeatureTable_v3::FeatureTable_v3(GameRules rules) :
			features(power(4, Feature::length(rules)))
	{
//		double t0 = getTime();
		init_features(rules);
//		double t1 = getTime();
		init_update_mask(rules);

//		double t2 = getTime();
//		std::cout << (t1 - t0) << " " << (t2 - t1) << std::endl;
//
//		size_t count_cross[7] = { 0, 0, 0, 0, 0, 0, 0 };
//		size_t count_circle[7] = { 0, 0, 0, 0, 0, 0, 0 };
//
//		for (size_t i = 0; i < features.size(); i++)
//		{
//			count_cross[static_cast<int>(getFeatureType(i).for_cross)]++;
//			count_circle[static_cast<int>(getFeatureType(i).for_circle)]++;
//		}
//
//		for (int i = 0; i < 7; i++)
//			std::cout << i << " : " << count_cross[i] << " " << count_circle[i] << '\n';
//
//		Feature base_line(Feature::length(rules));
//		size_t total_count = 0;
//		size_t empty_count = 0;
//		size_t update_count = 0;
//		for (size_t i = 0; i < features.size(); i++)
//		{
//			base_line.decode(i);
//			if (base_line.isValid())
//			{
//				total_count += base_line.size();
//				int empty = 0;
//				for (size_t j = 0; j < base_line.size(); j++)
//					empty += static_cast<int>(base_line.get(j) == Sign::NONE);
//				empty_count += empty;
//
//				FeatureEncoding enc = getFeatureType(i);
//				empty = 1;
//				for (size_t j = 0; j < base_line.size() - 1; j++)
//					empty += static_cast<int>(enc.mustBeUpdated(j));
//				update_count += empty;
//			}
//		}
//		std::cout << update_count << '\n' << empty_count << '\n' << total_count << "\n\n";
	}
	/*
	 * private
	 */
	void FeatureTable_v3::init_features(GameRules rules)
	{
		std::vector<bool> was_processed(features.size());

		const ThreatClassifier for_cross(rules, Sign::CROSS);
		const ThreatClassifier for_circle(rules, Sign::CIRCLE);

		Feature line(Feature::length(rules));

		for (size_t i = 0; i < features.size(); i++)
			if (was_processed[i] == false)
			{
				line.decode(i);
				if (line.isValid())
				{
					FeatureEncoding feature;
					line.setCenter(Sign::CROSS);
					feature.for_cross = for_cross(line);
					line.setCenter(Sign::CIRCLE);
					feature.for_circle = for_circle(line);
					line.setCenter(Sign::NONE);

					features[i] = feature.encode();
					was_processed[i] = true;
					line.flip(); // the features are symmetrical so we can update both at the same time
					features[line.encode()] = feature.encode();
					was_processed[line.encode()] = true;
				}
			}
	}
	void FeatureTable_v3::init_update_mask(GameRules rules)
	{
		std::vector<bool> was_processed(features.size());

		const int feature_length = Feature::length(rules);
		const int side_length = feature_length / 2;
		Feature base_line(feature_length);
		Feature secondary_line(feature_length);

		for (size_t i = 0; i < features.size(); i++)
		{
			base_line.decode(i);
			if (was_processed[i] == false and base_line.isValid())
			{
				FeatureEncoding feature = getFeatureType(i);
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
									const FeatureEncoding original = getFeatureType(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CROSS);
									const FeatureEncoding cross_altered = getFeatureType(secondary_line.encode());
									secondary_line.set(feature_length - 1 - spot_index, Sign::CIRCLE);
									const FeatureEncoding circle_altered = getFeatureType(secondary_line.encode());

									if (original.for_cross != cross_altered.for_cross or original.for_circle != cross_altered.for_circle
											or original.for_cross != circle_altered.for_cross or original.for_circle != circle_altered.for_circle)
									{
										must_be_updated = true;
										break;
									}
								}
							}
						const int dst_index = spot_index - static_cast<int>(spot_index > side_length); // center spot must be omitted
						feature.setUpdateMask(dst_index, must_be_updated);
					}
				features[i] = feature.encode();
				was_processed[i] = true;
				base_line.decode(i);

				base_line.flip(); // update mask is symmetrical so we can update both at the same time
				FeatureEncoding tmp(feature);
				for (int k = 0; k < feature_length - 1; k++) // flip update mask
					tmp.setUpdateMask(k, feature.mustBeUpdated(feature_length - 2 - k));
				features[base_line.encode()] = tmp.encode();
				was_processed[base_line.encode()] = true;
			}
		}
	}

} /* namespace ag */

