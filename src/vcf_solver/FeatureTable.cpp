/*
 * FeatureTable.cpp
 *
 *  Created on: 2 maj 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/rules/game_rules.hpp>

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

	bool is_feature_possible(const std::vector<Sign> &line)
	{
		for (size_t i = 1; i < line.size(); i++)
			if (line[i] == Sign::ILLEGAL and line[i - 1] != Sign::ILLEGAL)
				return false;
		return true;
	}

	struct FeatureClassifier
	{
		private:
			size_t length;
			GameRules rules;
			std::vector<Sign> line;

			void fill_line(FeatureDescriptor feature) noexcept
			{
				uint32_t tmp = feature.left;
				for (size_t j = 0; j < length; j++, tmp /= 4)
					line[j] = static_cast<Sign>(tmp & 3);
				line[length] = Sign::NONE;
				tmp = feature.right;
				for (size_t j = 0; j < length; j++, tmp /= 4)
					line[line.size() - 1 - j] = static_cast<Sign>(tmp & 3);
			}
			bool is_five_for(Sign sign) noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				assert(line[length] == Sign::NONE);
				line[length] = sign;
				const GameOutcome outcome = getOutcome(rules, line);
				line[length] = Sign::NONE;
				if (sign == Sign::CROSS)
					return outcome == GameOutcome::CROSS_WIN;
				else
					return outcome == GameOutcome::CIRCLE_WIN;
			}
			int is_four_for(Sign sign) noexcept
			{
				int winning = 0;
				for (size_t i = 0; i < line.size(); i++)
					if (line[i] == Sign::NONE)
					{
						line[i] = sign;
						if (is_five_for_cross(line, rules))
							winning++;
						line[i] = Sign::NONE;
					}
				return winning;
			}
			ThreatType get_threat_type_for(Sign sign) noexcept
			{
				if (is_five_for(sign))
					return ThreatType::FIVE;
				else
				{
					switch (is_four_for(sign))
					{
						case 0:
							return ThreatType::NONE;
						case 1:
							return ThreatType::HALF_OPEN_FOUR;
						default:
							return ThreatType::OPEN_FOUR;
					}
				}
			}
		public:
			FeatureClassifier(GameRules rules) :
					length(get_feature_length(rules)),
					rules(rules),
					line(2 * length + 1)
			{
			}
			Threat operator()(FeatureDescriptor feature)
			{
				fill_line(feature);

				Threat result;
				result.for_cross = get_threat_type_for(Sign::CROSS);
				result.for_cross = get_threat_type_for(Sign::CIRCLE);
				return result;
			}
	};
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
		create_maps(rules);
		init(rules);
		init_v2(rules);
	}
	Threat FeatureTable::getThreat(uint32_t feature) const noexcept
	{
		return Threat(features[feature]);
	}
	Threat FeatureTable::getThreat(FeatureDescriptor feature) const noexcept
	{
		return Threat(features_v2[get_position(feature)]);
	}
	/*
	 * private
	 */
	void FeatureTable::init(GameRules rules)
	{
		const size_t line_length = 2 * get_feature_length(rules) + 1;
		const size_t number_of_features = 1 << (2 * line_length);
		features.reserve(number_of_features);

		std::vector<Sign> feature(line_length);
		for (size_t i = 0; i < number_of_features; i++)
		{
			decode_feature(feature, i);
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
			features[i] = result.encode();
		}
	}
	void FeatureTable::init_v2(GameRules rules)
	{
		features_v2.reserve(legal_features * (legal_features + 1) / 2);

		const uint16_t number_of_features = 1 << (2 * feature_length);

		FeatureClassifier classify(rules);
		int counter = 0;
		for (uint16_t left = 0; left < number_of_features; left++)
			for (uint16_t right = 0; right <= left; right++)
				if (index_map[left] != -1 and index_map[right] != -1) //both sides are possible
				{
					FeatureDescriptor feature { left, right };
					Threat threat = classify(feature);
					const int position = get_position(feature);
					features_v2[position] = threat.encode();
					counter++;
				}
	}
	void FeatureTable::create_maps(GameRules rules)
	{
		const size_t number_of_features = 1 << (2 * feature_length);
		std::vector<Sign> feature(feature_length);

		legal_features = 0;
		index_map.reserve(number_of_features);
		for (size_t i = 0; i < number_of_features; i++)
		{
			decode_feature(feature, i);
			if (is_feature_possible(feature))
			{
				index_map[i] = legal_features;
				legal_features++;
			}
			else
				index_map[i] = -1;
		}
	}
	int FeatureTable::get_position(FeatureDescriptor feature) const noexcept
	{
		const int row = index_map[feature.left];
		const int col = index_map[feature.right];
		assert(row != -1 && col != -1);
		if (row >= col)
			return (row + 1) * row / 2 + col;
		else
			return (col + 1) * col / 2 + row;
	}
} /* namespace ag */

