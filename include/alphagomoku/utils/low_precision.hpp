/*
 * low_precision.hpp
 *
 *  Created on: Aug 13, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_LOW_PRECISION_HPP_
#define ALPHAGOMOKU_UTILS_LOW_PRECISION_HPP_

#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/bit_utils.hpp>

#include <cinttypes>
#include <algorithm>

namespace ag
{
	template<int S, int E, int M, int B>
	class LowFP
	{
			static_assert(S == 0 || S == 1, "there must be at most one sign bit");
			static_assert((S + E + M) <= 32, "number of bits must not exceed 32");

			static constexpr uint32_t ones(uint32_t n) noexcept
			{
				return (1u << n) - 1u;
			}

			static constexpr int max_exponent = (1 << E) - 1 + B;
			static constexpr int min_exponent = B;
			static constexpr uint32_t max_mantissa = (1u << M) - 1u;

			static constexpr uint32_t sign_mask = (S == 1) ? (1u << (E + M)) : 0x00u;
			static constexpr uint32_t exponent_mask = ones(E) << M;
			static constexpr uint32_t mantissa_mask = ones(M);

			uint32_t data = 0u;

			static float get_scale(int e) noexcept
			{
				return (e < 0) ? static_cast<float>(1 << (-e)) : 1.0f / (1 << e);
			}
		public:
			LowFP() noexcept = default;
			LowFP(float x) noexcept :
					data(to_lowp(x))
			{
			}
			LowFP(double x) noexcept :
					data(to_lowp(static_cast<float>(x)))
			{
			}
			LowFP(int x) noexcept :
					data(to_lowp(static_cast<float>(x)))
			{
			}

			LowFP& operator=(float x) noexcept
			{
				this->data = to_lowp(x);
				return *this;
			}
			LowFP& operator=(double x) noexcept
			{
				this->data = to_lowp(static_cast<float>(x));
				return *this;
			}
			LowFP& operator=(int x) noexcept
			{
				this->data = to_lowp(static_cast<float>(x));
				return *this;
			}

			operator float() const noexcept
			{
				return to_fp32(data);
			}
			operator double() const noexcept
			{
				return static_cast<double>(to_fp32(data));
			}
			operator int() const noexcept
			{
				return static_cast<int>(to_fp32(data));
			}

			static constexpr int bitsize() noexcept
			{
				return S + E + M;
			}

			static float max() noexcept
			{
				return to_fp32((S == 0) ? ones(bitsize()) : ones(bitsize() - 1));
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
				return to_fp32((S == 0) ? 0 : ones(bitsize()));
			}

			static uint32_t to_lowp(float x) noexcept
			{
				const uint32_t bits = bitwise_cast<uint32_t>(x);
				const uint32_t sign = (S == 1) ? ((bits & 0x80000000u) >> (32u - bitsize())) : 0u;
				const int exponent = clamp(static_cast<int>((bits & 0x7F800000u) >> 23u) - 127, min_exponent, max_exponent);
				const int is_subnormal = (exponent == min_exponent) ? 1 : 0;
				const float base = ((sign == 0) ? x : (-x)) * get_scale(exponent + is_subnormal) + is_subnormal - 1;
				const uint32_t mantissa = std::min(max_mantissa, static_cast<uint32_t>(base * (1 << M) + 0.5f));
				return sign | (static_cast<uint32_t>(exponent - B) << M) | mantissa;
			}
			static float to_fp32(uint32_t x) noexcept
			{
				if constexpr (bitsize() <= 10)
				{
					static const std::vector<float> table = init_fp32_conversion_table();
					return table[x];
				}
				else
					return convert_to_fp32(x);
			}

			static uint32_t to_lowp(int x) noexcept
			{
				return to_lowp(static_cast<float>(x));
			}
			static int to_int32(uint32_t x) noexcept
			{
				return static_cast<int>(to_fp32(x));
			}
		private:
			static float convert_to_fp32(uint32_t x) noexcept
			{
				const uint32_t sign = (S == 1) ? (x & sign_mask) : 0u;
				const int exponent = ((static_cast<uint32_t>(x) & exponent_mask) >> M) + B;
				const float base = static_cast<float>(x & mantissa_mask) / (1 << M);
				const int is_subnormal = (exponent == min_exponent) ? 1 : 0;
				return ((sign == 0) ? 1.0f : -1.0f) * (1 - is_subnormal + base) / get_scale(exponent + is_subnormal);
			}
			static std::vector<float> init_fp32_conversion_table()
			{
				std::vector<float> result(1 << bitsize());
				for (uint32_t i = 0; i < result.size(); i++)
					result[i] = convert_to_fp32(i);
				return result;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_LOW_PRECISION_HPP_ */
