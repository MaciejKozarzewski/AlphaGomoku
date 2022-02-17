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

	struct Threat
	{
			ThreatType for_cross = ThreatType::NONE;
			ThreatType for_circle = ThreatType::NONE;
			Threat() = default;
			Threat(ThreatType cross, ThreatType circle) :
					for_cross(cross),
					for_circle(circle)
			{
			}
	};

	class FeatureTable
	{
		private:
			std::vector<uint8_t> features;
		public:
			FeatureTable(GameRules rules);
			Threat getThreat(uint32_t feature) const noexcept;
		private:
			void init(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_ */
