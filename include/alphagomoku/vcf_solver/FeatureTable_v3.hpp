/*
 * FeatureTable_v3.hpp
 *
 *  Created on: Apr 9, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V3_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V3_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

#include <cassert>

namespace ag
{
	enum class FeatureType : int8_t
	{
		NONE,
		OPEN_3,
		HALF_OPEN_4,
		OPEN_4,
		FORK_4x4,
		FIVE, // actually it is a winning pattern, so for some rules it means 'five but not overline'
		OVERLINE
	};

	struct FeatureEncoding
	{
		private:
			uint16_t update_mask = 0u;
		public:
			FeatureType for_cross = FeatureType::NONE;
			FeatureType for_circle = FeatureType::NONE;

			FeatureEncoding() noexcept = default;
			FeatureEncoding(uint16_t encoding) noexcept :
					update_mask(encoding & 1023),
					for_cross(static_cast<FeatureType>((encoding >> 10) & 7)),
					for_circle(static_cast<FeatureType>((encoding >> 13) & 7))
			{
			}
			FeatureEncoding(FeatureType cross, FeatureType circle) noexcept :
					for_cross(cross),
					for_circle(circle)
			{
			}
			uint16_t encode() const noexcept
			{
				return (update_mask & 1023) | (static_cast<uint16_t>(for_cross) << 10) | (static_cast<uint16_t>(for_circle) << 13);
			}
			bool mustBeUpdated(int index) const noexcept
			{
				assert(index >= 0 && index < 10);
				return static_cast<bool>((update_mask >> index) & 1);
			}
			void setUpdateMask(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 10);
				update_mask &= (~(1 << index));
				update_mask |= (static_cast<uint16_t>(b) << index);
			}
	};

	class FeatureTable_v3
	{
		private:
			std::vector<uint16_t> features;
		public:
			FeatureTable_v3(GameRules rules);
			FeatureEncoding getFeatureType(uint32_t feature) const noexcept
			{
				assert(feature < features.size());
				return FeatureEncoding(features[feature]);
			}
		private:
			void init_features(GameRules rules);
			void init_update_mask(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V3_HPP_ */
