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
			std::vector<Sign> m_data;
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
			Feature(size_t len) :
					m_data(len, Sign::NONE)
			{
			}
			Feature(size_t len, uint32_t feature) :
					Feature(len)
			{
				decode(feature);
			}
			Feature(const std::string &str) :
					m_data(str.size(), Sign::NONE)
			{
				for (size_t i = 0; i < m_data.size(); i++)
					m_data[i] = signFromText(str[i]);
			}
			bool isValid() const noexcept
			{
				if (center() != Sign::NONE)
					return false;
				for (size_t i = 0; i < m_data.size() / 2; i++)
					if (m_data[i] != Sign::ILLEGAL and m_data[i + 1] == Sign::ILLEGAL)
						return false;
				for (size_t i = 1 + m_data.size() / 2; i < m_data.size(); i++)
					if (m_data[i - 1] == Sign::ILLEGAL and m_data[i] != Sign::ILLEGAL)
						return false;
				return true;
			}
			void invert() noexcept
			{
				for (size_t i = 0; i < m_data.size(); i++)
					m_data[i] = invertSign(m_data[i]);
			}
			void flip() noexcept
			{
				std::reverse(m_data.begin(), m_data.end());
			}
			Sign center() const noexcept
			{
				return m_data[m_data.size() / 2];
			}
			Sign& center() noexcept
			{
				return m_data[m_data.size() / 2];
			}
			Sign operator[](size_t index) const noexcept
			{
				assert(index < m_data.size());
				return m_data[index];
			}
			Sign& operator[](size_t index) noexcept
			{
				assert(index < m_data.size());
				return m_data[index];
			}
			void decode(uint32_t feature) noexcept
			{
				for (size_t i = 0; i < m_data.size(); i++, feature /= 4)
					m_data[i] = static_cast<Sign>(feature & 3);
			}
			uint32_t encode() const noexcept
			{
				uint32_t result = 0;
				for (size_t i = 0; i < m_data.size(); i++)
					result |= (static_cast<int>(m_data[i]) << (2 * i));
				return result;
			}
			std::string toString() const
			{
				std::string result;
				for (size_t i = 0; i < m_data.size(); i++)
					result += text(m_data[i]);
				return result;
			}
			size_t size() const noexcept
			{
				return m_data.size();
			}
			void shiftLeft(size_t n) noexcept
			{
				if (n > size())
					std::fill(m_data.begin(), m_data.end(), Sign::NONE);
				else
				{
					size_t i = 0;
					for (; i < m_data.size() - n; i++)
						m_data[i] = m_data[i + n];
					std::fill(m_data.begin() + i, m_data.end(), Sign::NONE);
				}
			}
			void shiftRight(size_t n) noexcept
			{
				if (n > size())
					std::fill(m_data.begin(), m_data.end(), Sign::NONE);
				else
				{
					for (size_t i = m_data.size() - 1; i >= n; i--)
						m_data[i] = m_data[i - n];
					std::fill(m_data.begin(), m_data.begin() + n, Sign::NONE);
				}
			}
			void mergeWith(const Feature &other) noexcept
			{
				assert(this->size() == other.size());
				for (size_t i = 0; i < m_data.size(); i++)
				{
					assert(m_data[i] == other[i] || (m_data[i] == Sign::NONE || other[i] == Sign::NONE));
					m_data[i] = static_cast<Sign>(static_cast<int>(m_data[i]) | static_cast<int>(other[i]));
				}
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
						is_a_match &= m_allowed_values[j][static_cast<int>(f[i + j])];
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
					return FeatureType::FORK_4x4;
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
			feature_length(Feature::length(rules))
	{
		double t0 = getTime();
		init_features(rules);
		double t1 = getTime();
		init_update_mask(rules);
		double t2 = getTime();
		std::cout << (t1 - t0) << " " << (t2 - t1) << std::endl;
	}
	/*
	 * private
	 */
	void FeatureTable_v3::init_features(GameRules rules)
	{
		const ThreatClassifier for_cross(rules, Sign::CROSS);
		const ThreatClassifier for_circle(rules, Sign::CIRCLE);

		features.resize(power(4, Feature::length(rules)));

		Feature line(Feature::length(rules));

		for (size_t i = 0; i < features.size(); i++)
		{
			line.decode(i);
			if (line.isValid())
			{
				FeatureEncoding feature;
				line.center() = Sign::CROSS;
				feature.for_cross = for_cross(line);
				line.center() = Sign::CIRCLE;
				feature.for_circle = for_circle(line);
				line.center() = Sign::NONE;
				features[i] = feature.encode();
			}
		}
	}
	void FeatureTable_v3::init_update_mask(GameRules rules)
	{
		const int feature_length = Feature::length(rules);
		const int side_length = feature_length / 2;
		Feature base_line(feature_length);
		Feature secondary_line(feature_length);

		for (size_t i = 0; i < features.size(); i++)
		{
			base_line.decode(i);

			FeatureEncoding feature = getFeatureType(i);
			if (base_line.isValid())
			{
				for (int spot_index = 0; spot_index < feature_length; spot_index++)
					if (spot_index != side_length)
					{
						const int free_spots = std::abs(side_length - spot_index);

						base_line.decode(i);
						if (spot_index < side_length)
							base_line.shiftRight(free_spots);
						else
							base_line.shiftLeft(free_spots);

						bool must_be_updated = false;
						if (base_line.center() == Sign::NONE)
							for (int j = 0; j < power(4, free_spots); j++)
							{
								secondary_line.decode(j);
								if (spot_index > side_length)
									secondary_line.flip();
								secondary_line.mergeWith(base_line);
								if (secondary_line.isValid())
								{
									const uint32_t encoding = secondary_line.encode();
									const uint32_t shift = 2 * (secondary_line.size() - 1 - spot_index);

									const FeatureEncoding original = getFeatureType(encoding);
									const FeatureEncoding cross_altered = getFeatureType(encoding | (static_cast<uint32_t>(Sign::CROSS) << shift));
									const FeatureEncoding circle_altered = getFeatureType(encoding | (static_cast<uint32_t>(Sign::CIRCLE) << shift));

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
			}
			features[i] = feature.encode();
		}

		size_t total_count = 0;
		size_t empty_count = 0;
		size_t update_count = 0;
		for (size_t i = 0; i < features.size(); i++)
		{
			base_line.decode(i);
			if (base_line.isValid())
			{
				total_count += base_line.size();
				int empty = 0;
				for (size_t j = 0; j < base_line.size(); j++)
					empty += static_cast<int>(base_line[j] == Sign::NONE);
				empty_count += empty;

				FeatureEncoding enc = getFeatureType(i);
				empty = 1;
				for (size_t j = 0; j < base_line.size() - 1; j++)
					empty += static_cast<int>(enc.mustBeUpdated(j));
				update_count += empty;
			}
		}
		std::cout << update_count << std::endl << empty_count << std::endl << total_count << std::endl;
	}
} /* namespace ag */

