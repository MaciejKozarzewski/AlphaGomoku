/*
 * FeatureTable.hpp
 *
 *  Created on: 2 maj 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>

namespace ag
{
	enum class ThreatType
	{
		NONE,
		HALF_OPEN_FOUR,
		OPEN_FOUR,
		FIVE
	};

	std::string toString(ThreatType t);

	struct FeatureDescriptor
	{
			uint16_t left;
			uint16_t right;
	};

	struct Threat
	{
			ThreatType for_cross = ThreatType::NONE;
			ThreatType for_circle = ThreatType::NONE;
			Threat() = default;
			Threat(uint8_t encoding) :
					for_cross(static_cast<ThreatType>(encoding & 15)),
					for_circle(static_cast<ThreatType>((encoding >> 4) & 15))
			{
			}
			Threat(ThreatType cross, ThreatType circle) :
					for_cross(cross),
					for_circle(circle)
			{
			}
			uint8_t encode() const noexcept
			{
				return static_cast<uint32_t>(for_cross) | (static_cast<uint32_t>(for_circle) << 4);
			}
	};

	class FeatureTable
	{
		private:
			int feature_length;
			int legal_features;
			std::vector<uint8_t> features;
			std::vector<uint8_t> features_v2;
			std::vector<int> index_map;
		public:
			FeatureTable(GameRules rules);
			Threat getThreat(uint32_t feature) const noexcept;
			Threat getThreat(FeatureDescriptor feature) const noexcept;
		private:
			void init(GameRules rules);
			void init_v2(GameRules rules);
			void create_maps(GameRules rules);
			int get_position(FeatureDescriptor feature) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_ */
