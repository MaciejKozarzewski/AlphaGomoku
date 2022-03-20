/*
 * FeatureTable_v2.cpp
 *
 *  Created on: Mar 18, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureTable_v2.hpp>

#include <iostream>
#include <algorithm>
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

	template<int Length>
	class Feature
	{
		private:
			uint32_t line = 0u;
		public:
			constexpr Feature() = default;
			constexpr Feature(uint32_t feature) noexcept :
					line(feature)
			{
			}
			constexpr Feature(const char *feature) :
					line(0u)
			{
				assert(strlen(feature) == Length);
				for (int i = 0; i < Length; i++)
				{
					Sign sign = signFromText(feature[i]);
					line = line | (static_cast<uint32_t>(sign) << (2 * i));
				}
			}
			std::string toString() const
			{
				std::string result;
				for (int i = 0; i < Length; i++)
					result += text(this->operator [](i));
				return result;
			}
			constexpr void shiftRight(uint32_t shift) noexcept
			{
				line = line >> (2u * shift);
			}
			constexpr void shiftLeft(uint32_t shift) noexcept
			{
				line = line << (2u * shift);
			}
			constexpr operator uint32_t() const noexcept
			{
				return line;
			}
			constexpr Sign operator[](int index) const noexcept
			{
				assert(index >= 0 && index < Length);
				return static_cast<Sign>((line >> (2 * index)) & 3);
			}
			constexpr int length() const noexcept
			{
				return Length;
			}
			void set_at_center(const Sign s) noexcept
			{
				const uint32_t mask = ~(3 << (Length - 1));
				const uint32_t insert = static_cast<uint32_t>(s) << (Length - 1);
				line = (line & mask) | insert;
			}
	};

	template<int L1, int L2>
	bool is_match(Feature<L1> feature, const Feature<L2> pattern) noexcept
	{
		const uint32_t mask = (1u << (2u * pattern.length())) - 1u;
		bool result = false;
		for (int i = 0; i <= feature.length() - pattern.length(); i++, feature.shiftRight(1))
			result |= (static_cast<uint32_t>(feature) & mask) == static_cast<uint32_t>(pattern);
		return result;
	}
	template<int Diff, int Length>
	Feature<Length - Diff> shrink_by(const Feature<Length> feature) noexcept
	{
		assert(Diff % 2 == 0);
		const uint32_t mask = (1 << (2 * (Length - Diff))) - 1;
		const uint32_t tmp = static_cast<uint32_t>(feature) >> Diff;
		return Feature<Length - Diff>(tmp & mask);
	}
	template<int Length>
	Feature<Length + 2> extend(const Sign lhs, const Feature<Length> feature, const Sign rhs) noexcept
	{
		const uint32_t left = static_cast<uint32_t>(lhs);
		const uint32_t center = static_cast<uint32_t>(feature) << 2;
		const uint32_t right = static_cast<uint32_t>(rhs) << (2 * Length + 2);
		return Feature<Length + 2>(left | center | right);
	}
	template<int Length>
	Feature<Length + 1> extend(const Sign lhs, const Feature<Length> feature) noexcept
	{
		const uint32_t left = static_cast<uint32_t>(lhs);
		const uint32_t center = static_cast<uint32_t>(feature) << 2;
		return Feature<Length + 1>(left | center);
	}
	template<int Length>
	Feature<Length + 1> extend(const Feature<Length> feature, const Sign rhs) noexcept
	{
		const uint32_t center = static_cast<uint32_t>(feature);
		const uint32_t right = static_cast<uint32_t>(rhs) << (2 * Length);
		return Feature<Length + 1>(center | right);
	}

	constexpr int length(GameRules rules) noexcept
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

	template<GameRules Rules, Sign SignToMove, int Length = (Rules == GameRules::FREESTYLE) ? 9 : 11>
	class FeatureClassifier
	{
		public:
			bool isFive(Feature<Length> feature) const noexcept
			{
				feature.set_at_center(SignToMove);

				constexpr Feature<5> five((SignToMove == Sign::CROSS) ? "XXXXX" : "OOOOO");
				constexpr Feature<6> overline((SignToMove == Sign::CROSS) ? "XXXXXX" : "OOOOOO");
				constexpr Feature<7> blocked_five((SignToMove == Sign::CROSS) ? "OXXXXXO" : "XOOOOOX");

				switch (Rules)
				{
					case GameRules::FREESTYLE:
						return is_match(feature, five);
					case GameRules::STANDARD:
					{
						const bool is_five = is_match(shrink_by<2>(feature), five);
						const bool is_overline = is_match(feature, overline);
						return is_five and not is_overline;
					}
					case GameRules::RENJU:
					{
						const bool is_five = is_match(shrink_by<2>(feature), five);
						if (SignToMove == Sign::CROSS)
						{
							const bool is_overline = is_match(feature, overline);
							return is_five and not is_overline;
						}
						else
							return is_five;
					}
					case GameRules::CARO:
					{
						const bool is_five = is_match(shrink_by<2>(feature), five);
						const bool is_blocked_five = is_match(feature, blocked_five);
						return is_five and not is_blocked_five;
					}
					default:
						return false;
				}
			}
			bool isOpenFour(Feature<Length> feature) const noexcept
			{
				feature.set_at_center(SignToMove);

				constexpr Feature<6> open_four((SignToMove == Sign::CROSS) ? "_XXXX_" : "_OOOO_");
				constexpr Feature<8> overline((SignToMove == Sign::CROSS) ? "X_XXXX_X" : "O_OOOO_O");
				constexpr Feature<8> blocked((SignToMove == Sign::CROSS) ? "O_XXXX_O" : "X_OOOO_X");
				switch (Rules)
				{
					case GameRules::FREESTYLE:
						return is_match(feature, open_four);
					case GameRules::STANDARD:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_overline = is_match(feature, overline);
						return is_five and not is_overline;
					}
					case GameRules::RENJU:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						if (SignToMove == Sign::CROSS)
						{
							const bool is_overline = is_match(feature, overline);
							return is_five and not is_overline;
						}
						else
							return is_five;
					}
					case GameRules::CARO:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_blocked_five = is_match(feature, blocked);
						return is_five and not is_blocked_five;
					}
					default:
						return false;
				}
			}
			bool isDoubleFour(Feature<Length> feature) const noexcept
			{
				feature.set_at_center(SignToMove);

				constexpr Feature<6> open_four((SignToMove == Sign::CROSS) ? "_XXXX_" : "_OOOO_");
				constexpr Feature<8> overline((SignToMove == Sign::CROSS) ? "X_XXXX_X" : "O_OOOO_O");
				constexpr Feature<8> blocked((SignToMove == Sign::CROSS) ? "O_XXXX_O" : "X_OOOO_X");
				switch (Rules)
				{
					case GameRules::FREESTYLE:
						return is_match(feature, open_four);
					case GameRules::STANDARD:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_overline = is_match(feature, overline);
						return is_five and not is_overline;
					}
					case GameRules::RENJU:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						if (SignToMove == Sign::CROSS)
						{
							const bool is_overline = is_match(feature, overline);
							return is_five and not is_overline;
						}
						else
							return is_five;
					}
					case GameRules::CARO:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_blocked_five = is_match(feature, blocked);
						return is_five and not is_blocked_five;
					}
					default:
						return false;
				}
			}
			bool isHalfOpenFour(Feature<Length> feature) const noexcept
			{
				feature.set_at_center(SignToMove);

				constexpr Feature<6> open_four((SignToMove == Sign::CROSS) ? "_XXXX_" : "_OOOO_");
				constexpr Feature<8> overline((SignToMove == Sign::CROSS) ? "X_XXXX_X" : "O_OOOO_O");
				constexpr Feature<8> blocked((SignToMove == Sign::CROSS) ? "O_XXXX_O" : "X_OOOO_X");
				switch (Rules)
				{
					case GameRules::FREESTYLE:
						return is_match(feature, open_four);
					case GameRules::STANDARD:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_overline = is_match(feature, overline);
						return is_five and not is_overline;
					}
					case GameRules::RENJU:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						if (SignToMove == Sign::CROSS)
						{
							const bool is_overline = is_match(feature, overline);
							return is_five and not is_overline;
						}
						else
							return is_five;
					}
					case GameRules::CARO:
					{
						const bool is_five = is_match(shrink_by<2>(feature), open_four);
						const bool is_blocked_five = is_match(feature, blocked);
						return is_five and not is_blocked_five;
					}
					default:
						return false;
				}
			}
	};

	template<Sign sign>
	ThreatType get_threat(uint32_t feature)
	{
		FeatureClassifier<GameRules::STANDARD, sign> classifier;
		if (classifier.isFive(feature))
			return ThreatType::FIVE;
		else
//		{
//			if (classifier.isOpenFour(feature))
//				return ThreatType::OPEN_FOUR;
//			else
			return ThreatType::NONE;
//		}
	}
}

namespace ag
{

	FeatureTable_v2::FeatureTable_v2(GameRules rules) :
			feature_length(length(rules))
	{
//		init(rules);
	}
	Threat FeatureTable_v2::getThreat(uint32_t feature) const noexcept
	{
		ThreatType cross = get_threat<Sign::CROSS>(feature);
		ThreatType circle = get_threat<Sign::CIRCLE>(feature);
		return Threat(cross, circle);
//		return Threat(features[feature]);
	}
	/*
	 * private
	 */
	void FeatureTable_v2::init(GameRules rules)
	{
		FeatureTable table(rules);

		FeatureClassifier<GameRules::FREESTYLE, Sign::CROSS> cross_features;
		FeatureClassifier<GameRules::FREESTYLE, Sign::CIRCLE> circle_features;

		const size_t number_of_features = 1 << (2 * length(rules));
		features.resize(number_of_features);

		for (size_t i = 0; i < number_of_features; i++)
		{
			Feature<9> feature(i);

			Threat result;
			if (feature[feature.length() / 2] == Sign::NONE)
			{
				if (cross_features.isFive(feature))
					result.for_cross = ThreatType::FIVE;

				if (circle_features.isFive(feature))
					result.for_circle = ThreatType::FIVE;
			}
			features.at(i) = result.encode();

			Threat t1 = table.getThreat(i);
			if (t1.for_cross != ThreatType::FIVE)
				t1.for_cross = ThreatType::NONE;
			if (t1.for_circle != ThreatType::FIVE)
				t1.for_circle = ThreatType::NONE;
			if (t1.for_cross != result.for_cross or t1.for_circle != result.for_circle)
			{
				std::cout << "error for feature " << i << '\n';
				std::cout << feature.toString() << '\n';
				std::cout << "correct = " << toString(t1.for_cross) << " : " << toString(t1.for_circle) << '\n';
				std::cout << "current = " << toString(result.for_cross) << " : " << toString(result.for_circle) << '\n';
				return;
			}
		}
	}

} /* namespace ag */

