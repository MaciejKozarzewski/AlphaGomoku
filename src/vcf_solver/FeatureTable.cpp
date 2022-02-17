/*
 * FeatureTable.cpp
 *
 *  Created on: 2 maj 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/rules/game_rules.hpp>

#include <iostream>

namespace
{
	uint32_t get_feature_length(ag::GameRules rules)
	{
		switch (rules)
		{
			case ag::GameRules::FREESTYLE:
				return 9;
			case ag::GameRules::STANDARD:
			case ag::GameRules::RENJU:
			case ag::GameRules::CARO:
				return 11;
			default:
				return 0;
		}
	}
	void fill_feature_line(std::vector<ag::Sign> &line, uint32_t feature)
	{
		for (size_t j = 0; j < line.size(); j++, feature /= 4)
			line[j] = static_cast<ag::Sign>(feature % 4);
	}
	std::string feature_to_string(const std::vector<ag::Sign> &line)
	{
		std::string result;
		for (size_t j = 0; j < line.size(); j++)
			switch (static_cast<int>(line[j]))
			{
				case 0:
					result += '_';
					break;
				case 1:
					result += 'X';
					break;
				case 2:
					result += 'O';
					break;
				case 3:
					result += '|';
					break;
			}
		return result;
	}
	bool is_five_for_cross(const std::vector<ag::Sign> &line, ag::GameRules rules)
	{
		return ag::getOutcome(rules, line) == ag::GameOutcome::CROSS_WIN;
	}
	bool is_five_for_circle(const std::vector<ag::Sign> &line, ag::GameRules rules)
	{
		return ag::getOutcome(rules, line) == ag::GameOutcome::CIRCLE_WIN;
	}
	int is_four_for_cross(std::vector<ag::Sign> &line, ag::GameRules rules)
	{
		int winning = 0;
		for (size_t i = 0; i < line.size(); i++)
			if (line[i] == ag::Sign::NONE)
			{
				line[i] = ag::Sign::CROSS;
				if (is_five_for_cross(line, rules))
					winning++;
				line[i] = ag::Sign::NONE;
			}
		return winning;
	}
	int is_four_for_circle(std::vector<ag::Sign> &line, ag::GameRules rules)
	{
		int winning = 0;
		for (size_t i = 0; i < line.size(); i++)
			if (line[i] == ag::Sign::NONE)
			{
				line[i] = ag::Sign::CIRCLE;
				if (is_five_for_circle(line, rules))
					winning++;
				line[i] = ag::Sign::NONE;
			}
		return winning;
	}
}

namespace ag
{

	std::string toString(ThreatType t)
	{
		switch (t)
		{
			default:
			case ThreatType::NONE:
				return "NONE";
			case ThreatType::HALF_OPEN_FOUR:
				return "HALF_OPEN_FOUR";
			case ThreatType::OPEN_FOUR:
				return "OPEN_FOUR";
			case ThreatType::FIVE:
				return "FIVE";
		}
	}

	FeatureTable::FeatureTable(GameRules rules)
	{
		init(rules);
	}
	Threat FeatureTable::getThreat(uint32_t feature) const noexcept
	{
		Threat result;
		result.for_cross = static_cast<ThreatType>((features[feature] & 15));
		result.for_circle = static_cast<ThreatType>((features[feature] >> 4) & 15);
		return result;
	}
	void FeatureTable::init(GameRules rules)
	{
		uint32_t number_of_features = 1 << (2 * get_feature_length(rules));
		features.reserve(number_of_features);

		std::vector<Sign> feature(get_feature_length(rules));
		for (uint32_t i = 0; i < number_of_features; i++)
		{
			fill_feature_line(feature, i);
			Threat result;
			if (feature[feature.size() / 2] == Sign::NONE)
			{
				feature[feature.size() / 2] = Sign::CROSS;
				if (is_five_for_cross(feature, rules))
					result.for_cross = ThreatType::FIVE;
				else
				{
					switch (is_four_for_cross(feature, rules))
					{
						case 0:
							break;
						case 1:
							result.for_cross = ThreatType::HALF_OPEN_FOUR;
							break;
						default:
							result.for_cross = ThreatType::OPEN_FOUR;
							break;
					}
				}

				feature[feature.size() / 2] = Sign::CIRCLE;
				if (is_five_for_circle(feature, rules))
					result.for_circle = ThreatType::FIVE;
				else
				{
					switch (is_four_for_circle(feature, rules))
					{
						case 0:
							break;
						case 1:
							result.for_circle = ThreatType::HALF_OPEN_FOUR;
							break;
						default:
							result.for_circle = ThreatType::OPEN_FOUR;
							break;
					}
				}
			}
			features[i] = static_cast<uint32_t>(result.for_cross) | (static_cast<uint32_t>(result.for_circle) << 4);
		}
	}

} /* namespace ag */

