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

#include <array>
#include <cassert>

namespace ag
{
	enum class FeatureType : int8_t
	{
		NONE,
		OPEN_3,
		HALF_OPEN_4,
		OPEN_4,
		DOUBLE_4,
		FIVE, // actually, it is a winning pattern so for some rules it means 'five but not overline'
		OVERLINE
	};
	std::string toString(FeatureType ft);

	struct FeatureTypeGroup
	{
			std::array<FeatureType, 4> for_cross;
			std::array<FeatureType, 4> for_circle;
	};

	struct FeatureEncoding
	{
		private:
			uint16_t m_data = 0u;
		public:
			FeatureEncoding() noexcept = default;
			FeatureEncoding(FeatureType cross, FeatureType circle) noexcept :
					m_data(static_cast<uint16_t>(cross) | (static_cast<uint16_t>(circle) << 3))
			{
			}
			FeatureType forCross() const noexcept
			{
				return static_cast<FeatureType>(m_data & 7);
			}
			FeatureType forCircle() const noexcept
			{
				return static_cast<FeatureType>((m_data >> 3) & 7);
			}
			bool mustBeUpdated(int index) const noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				return static_cast<bool>((m_data >> index) & 1);
			}
			void setUpdateMask(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				m_data &= (~(1 << index));
				m_data |= (static_cast<uint16_t>(b) << index);
			}
			bool isDefensiveMove(int index) const noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6 + 11;
				return static_cast<bool>((m_data >> index) & 1);
			}
			void setDefensiveMove(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6 + 11;
				m_data &= (~(1 << index));
				m_data |= (static_cast<uint16_t>(b) << index);
			}
	};

	class FeatureTable_v3
	{
		private:
			std::vector<FeatureEncoding> features;
		public:
			FeatureTable_v3(GameRules rules);
			FeatureEncoding getFeatureType(uint32_t feature) const noexcept
			{
				assert(feature < features.size());
				return features[feature];
			}
		private:
			void init_features(GameRules rules);
			void init_update_mask(GameRules rules);
			void init_defensive_moves(GameRules rules);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATURETABLE_V3_HPP_ */
