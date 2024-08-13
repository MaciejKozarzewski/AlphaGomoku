/*
 * fp8.hpp
 *
 *  Created on: Aug 13, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_FP8_HPP_
#define ALPHAGOMOKU_UTILS_FP8_HPP_

#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/bit_utils.hpp>

#include <cinttypes>
#include <algorithm>

namespace ag
{
	template<int S, int E, int M, int B>
	class float8
	{
			static_assert(S == 0 || S == 1, "there must be at most one sign bit");
			static_assert((S + E + M) <= 8, "number of bits must not exceed 8");

			static constexpr uint32_t ones(uint32_t n) noexcept
			{
				return (1u << n) - 1u;
			}

			static constexpr int max_exponent = (1 << E) - 1 + B;
			static constexpr int min_exponent = B;
			static constexpr uint32_t max_mantissa = (1u << M) - 1u;

			static constexpr uint32_t sign_mask = (S == 1) ? 0x80u : 0x00u;
			static constexpr uint32_t exponent_mask = ones(E) << M;
			static constexpr uint32_t mantissa_mask = ones(M);

			uint8_t data = 0u;

			static float get_scale(int e) noexcept
			{
				return (e < 0) ? static_cast<float>(1 << (-e)) : 1.0f / (1 << e);
			}
		public:
			float8() noexcept = default;
			float8(float x) noexcept :
					data(to_fp8(x))
			{
			}
			float8(double x) noexcept :
					data(to_fp8(x))
			{
			}
			float8& operator=(double x) noexcept
			{
				this->data = to_fp8(x);
				return *this;
			}
			float8& operator=(float x) noexcept
			{
				this->data = to_fp8(x);
				return *this;
			}
			operator float() const noexcept
			{
				return to_fp32(data);
			}

			static float max() noexcept
			{
				return to_fp32((S == 0) ? 255 : 127);
			}
			static float min() noexcept
			{
				return to_fp32(1 << M);
			}
			static float denorm_min() noexcept
			{
				return to_fp32(1);
			}
			static float lowest() noexcept
			{
				return to_fp32((S == 0) ? 0 : 255);
			}
			static float epsilon() noexcept
			{
				return 1.0f / (1 << M);
			}

			static uint8_t to_fp8(float x) noexcept
			{
				const uint32_t bits = bitwise_cast<uint32_t>(x);
				const uint32_t sign = (S == 1) ? ((bits & 0x80000000u) >> 24u) : 0u;
				const int exponent = clamp(static_cast<int>((bits & 0x7F800000u) >> 23u) - 127, min_exponent, max_exponent);
				const int is_subnormal = (exponent == min_exponent) ? 1 : 0;
				const float base = ((sign == 0) ? x : (-x)) * get_scale(exponent + is_subnormal) + is_subnormal - 1;
				const uint32_t mantissa = std::min(max_mantissa, static_cast<uint32_t>(base * (1 << M) + 0.5f));
				return sign | (static_cast<uint32_t>(exponent - B) << M) | mantissa;
			}
			static float to_fp32(uint8_t x) noexcept
			{
				static const std::vector<float> table = init_conversion_table();
				return table[x];
			}

			static std::vector<float> init_conversion_table()
			{
				std::vector<float> result(256);
				for (uint32_t i = 0; i < 256; i++)
				{
					const uint32_t sign = (S == 1) ? (i & sign_mask) : 0u;
					const int exponent = ((static_cast<uint32_t>(i) & exponent_mask) >> M) + B;
					const float base = static_cast<float>(i & mantissa_mask) / (1 << M);
					const int is_subnormal = (exponent == min_exponent) ? 1 : 0;
					result[i] = ((sign == 0) ? 1.0f : -1.0f) * (1 - is_subnormal + base) / get_scale(exponent + is_subnormal);
				}
				return result;
			}

	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_FP8_HPP_ */
