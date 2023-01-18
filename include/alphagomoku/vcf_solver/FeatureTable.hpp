/*
 * FeatureTable.hpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>

namespace ag::solver
{
	enum class Direction
	{
		HORIZONTAL,
		VERTICAL,
		DIAGONAL,
		ANTIDIAGONAL
	};

	enum class ThreatType
	{
		NONE,
		OPEN_THREE,
		HALF_OPEN_FOUR,
		OPEN_FOUR,
		FIVE,
		FORBIDDEN,
		DOUBLE_THREE,
		FOUR_THREE,
		DOUBLE_FOUR
	};

	std::string toString(ag::solver::ThreatType t);

	struct Threat
	{
			ag::solver::ThreatType for_cross = ag::solver::ThreatType::NONE;
			ag::solver::ThreatType for_circle = ag::solver::ThreatType::NONE;
			Threat() = default;
			Threat(uint8_t encoding) :
					for_cross(static_cast<ag::solver::ThreatType>(encoding & 15)),
					for_circle(static_cast<ag::solver::ThreatType>((encoding >> 4) & 15))
			{
			}
			Threat(ag::solver::ThreatType cross, ag::solver::ThreatType circle) :
					for_cross(cross),
					for_circle(circle)
			{
			}
			uint8_t encode() const noexcept
			{
				return static_cast<uint32_t>(for_cross) | (static_cast<uint32_t>(for_circle) << 4);
			}
			friend Threat max(const Threat &lhs, const Threat &rhs) noexcept
			{
				return Threat(std::max(lhs.for_cross, rhs.for_cross), std::max(lhs.for_circle, rhs.for_circle));
			}
	};

	class FeatureTable
	{
		private:
			int feature_length;
			int legal_features;
			std::vector<uint8_t> features;
		public:
			FeatureTable(GameRules rules);
			Threat getThreat(uint32_t feature) const noexcept
			{
				assert(feature < features.size());
				return Threat(features[feature]);
			}
		private:
			void init(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_HPP_ */
