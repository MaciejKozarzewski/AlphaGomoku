/*
 * FeatureValueTable.hpp
 *
 *  Created on: Apr 14, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREVALUETABLE_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREVALUETABLE_HPP_

#include <alphagomoku/game/Move.hpp>

#include <vector>
#include <cinttypes>

namespace ag::solver
{

	template<uint32_t N>
	class FeatureValueTable
	{
		private:
			struct InternalStorage
			{
					std::vector<uint64_t> statistics;
					std::vector<uint8_t> values;
					InternalStorage() :
							statistics(1 << (2 * N + 2)),
							values(statistics.size())
					{
					}
					void update(uint32_t rawFeature) noexcept
					{
//						raw_feature = (raw_feature >> (2 * pad - 4)) & 975;
					}
			};
			InternalStorage for_cross;
			InternalStorage for_circle;
		public:
			FeatureValueTable()
			{
			}
			void update(uint32_t rawFeature, Sign sign) noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					for_cross.update(rawFeature);
				else
					for_cross.update(rawFeature);
			}
	};
} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREVALUETABLE_HPP_ */
