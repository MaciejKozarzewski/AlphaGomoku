/*
 * FeatureTable_v2.hpp
 *
 *  Created on: Mar 18, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V2_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V2_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

namespace ag::solver
{
	class FeatureTable_v2
	{
		private:
			int feature_length;
			std::vector<uint8_t> features;
		public:
			FeatureTable_v2(GameRules rules);
			Threat getThreat(uint32_t feature) const noexcept;
		private:
			void init(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V2_HPP_ */
