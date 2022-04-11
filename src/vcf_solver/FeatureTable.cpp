/*
 * FeatureTable.cpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureTable.hpp>

#include <iostream>
#include <algorithm>

namespace
{
	using namespace ag;
	uint32_t get_feature_length(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return 4;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				return 5;
			default:
				return 0;
		}
	}
	void decode_feature(std::vector<Sign> &line, uint32_t feature)
	{
		for (size_t j = 0; j < line.size(); j++, feature /= 4)
			line[j] = static_cast<Sign>(feature & 3);
	}
	int encode_feature(const std::vector<Sign> &line)
	{
		int result = 0;
		for (size_t j = 0; j < line.size(); j++)
			result |= (static_cast<int>(line[j]) << (2 * j));
		return result;
	}
	bool is_valid(const std::vector<Sign> &line) noexcept
	{
		if (line[line.size() / 2] != Sign::NONE)
			return false;
		for (size_t i = 0; i < line.size() / 2; i++)
			if (line[i] != Sign::ILLEGAL and line[i + 1] == Sign::ILLEGAL)
				return false;
		for (size_t i = 1 + line.size() / 2; i < line.size(); i++)
			if (line[i - 1] == Sign::ILLEGAL and line[i] != Sign::ILLEGAL)
				return false;
		return true;
	}

	std::string feature_to_string(const std::vector<Sign> &line)
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
	bool is_five_for_cross(const std::vector<Sign> &line, GameRules rules)
	{
		return getOutcome(rules, line) == GameOutcome::CROSS_WIN;
	}
	bool is_five_for_circle(const std::vector<Sign> &line, GameRules rules)
	{
		return getOutcome(rules, line) == GameOutcome::CIRCLE_WIN;
	}
	int is_four_for_cross(std::vector<Sign> &line, GameRules rules)
	{
		int winning = 0;
		for (size_t i = 0; i < line.size(); i++)
			if (line[i] == Sign::NONE)
			{
				line[i] = Sign::CROSS;
				if (is_five_for_cross(line, rules))
					winning++;
				line[i] = Sign::NONE;
			}
		return winning;
	}
	int is_four_for_circle(std::vector<Sign> &line, GameRules rules)
	{
		int winning = 0;
		for (size_t i = 0; i < line.size(); i++)
			if (line[i] == Sign::NONE)
			{
				line[i] = Sign::CIRCLE;
				if (is_five_for_circle(line, rules))
					winning++;
				line[i] = Sign::NONE;
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

	FeatureTable::FeatureTable(GameRules rules) :
			feature_length(get_feature_length(rules))
	{
		init(rules);
	}
	/*
	 * private
	 */
	void FeatureTable::init(GameRules rules)
	{
		const size_t line_length = 2 * get_feature_length(rules) + 1;
		const size_t number_of_features = 1 << (2 * line_length);
		features.resize(number_of_features);

		std::vector<Sign> feature(line_length);
		for (size_t i = 0; i < number_of_features; i++)
		{
			decode_feature(feature, i);
			Threat result;
			if (is_valid(feature))
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
			features.at(i) = result.encode();
		}
	}
} /* namespace ag */

