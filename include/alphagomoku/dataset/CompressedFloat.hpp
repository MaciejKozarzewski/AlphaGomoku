/*
 * CompressedFloat.hpp
 *
 *  Created on: Feb 23, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_COMPRESSEDFLOAT_HPP_
#define ALPHAGOMOKU_DATASET_COMPRESSEDFLOAT_HPP_

#include <cinttypes>
#include <cassert>

namespace ag
{

	class CompressedFloat
	{
			static constexpr float scale = 65535.0f;
			static constexpr float inv_scale = 1.0f / 65535.0f;
			uint16_t raw = 0;
		public:
			CompressedFloat() noexcept = default;
			CompressedFloat(float x) noexcept :
					raw(static_cast<uint16_t>(scale * x))
			{
				assert(0.0f <= x && x <= 1.0f);
			}
			CompressedFloat& operator=(float x) noexcept
			{
				assert(0.0f <= x && x <= 1.0f);
				raw = static_cast<uint16_t>(scale * x);
				return *this;
			}
			operator float() const noexcept
			{
				return static_cast<float>(raw) * inv_scale;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_COMPRESSEDFLOAT_HPP_ */
