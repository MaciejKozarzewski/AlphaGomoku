/*
 * sse41_ops.cpp
 *
 *  Created on: Oct 23, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/nnue_ops/avx2_ops.hpp>

#include <x86intrin.h>
#include <cassert>
#include <iostream>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

	__m128i relu_8x16(__m128i x) noexcept
	{
		return _mm_max_epi16(_mm_setzero_si128(), x);
	}
	__m128i relu_4x32(__m128i x) noexcept
	{
		return _mm_max_epi32(_mm_setzero_si128(), x);
	}
	float horizontal_add(__m128 x) noexcept
	{
		const __m128 x64 = _mm_add_ps(x, _mm_movehl_ps(x, x));
		const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
		return _mm_cvtss_f32(x32);
	}

	template<int AccumulatorLength>
	struct RefreshAccumulator
	{
			static_assert(AccumulatorLength % 8 == 0);

			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &active) const noexcept
			{
				assert(layer_1.neurons() == AccumulatorLength);
				assert(accumulator.size() == AccumulatorLength);

				constexpr int length = AccumulatorLength / 8;
				__m128i acc[length]; // length x (16 x int16)
				for (int j = 0; j < length; j++)
					acc[j] = _mm_load_si128((const __m128i*) (layer_1.bias() + j * 8));
				for (size_t i = 0; i < active.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + active[i] * AccumulatorLength;
					for (int j = 0; j < length; j++, ptr += 8)
					{
						const __m128i row = _mm_loadu_si64(ptr); // loading 8 x int8
						const __m128i a = _mm_cvtepi8_epi16(row); // unpacked into 8 x int16
						acc[j] = _mm_add_epi16(acc[j], a);
					}
				}
				for (int j = 0; j < length; j++)
					_mm_store_si128((__m128i*) (accumulator.data() + j * 8), acc[j]);
			}
	};

	template<int AccumulatorLength>
	struct UpdateAccumulator
	{
			static_assert(AccumulatorLength % 8 == 0);

			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
					const std::vector<int> &added) const noexcept
			{
				constexpr int Length = AccumulatorLength / 8;

				__m128i acc[Length];
				for (int j = 0; j < Length; j++)
					acc[j] = _mm_load_si128((const __m128i*) (accumulator.data() + j * 8));
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + removed[i] * AccumulatorLength;
					for (int j = 0; j < Length; j++)
					{
						const __m128i row = _mm_loadu_si64((const __m128i*) (ptr + j * 8)); // loading entire row (32 x int8)
						const __m128i a = _mm_cvtepi8_epi16(row); // unpacked first half into 16 x int16
						acc[j] = _mm_sub_epi16(acc[j], a);
					}
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + added[i] * AccumulatorLength;
					for (int j = 0; j < Length; j++)
					{
						const __m128i row = _mm_loadu_si64((const __m128i*) (ptr + j * 8)); // loading entire row (32 x int8)
						const __m128i a = _mm_cvtepi8_epi16(row); // unpacked first half into 16 x int16
						acc[j] = _mm_add_epi16(acc[j], a);
					}
				}

				for (int j = 0; j < Length; j++)
					_mm_storeu_si128((__m128i*) (accumulator.data() + j * 8), acc[j]);
			}
	};

	template<int AccumulatorLength, int MiddleNeurons>
	struct Forward
	{
			float run(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_2,
					const NnueLayer<float, float> &layer_3) const noexcept
			{
				/*
				 * accumulator is loaded and broadcasted as 2xint16 (a0, a1)
				 * 														/col 0 \  /col 1 \
				 * weights are loaded as columns interleaved by 2 rows (b00, b10, b01, b11, ...)
				 * 														\2 rows/  \2 rows/
				 */

				constexpr int MiddleSize = MiddleNeurons / 4;

				__m128i acc[MiddleSize];
				for (int j = 0; j < MiddleSize; j++)
					acc[j] = _mm_load_si128((const __m128i*) (layer_2.bias() + j * 4)); // load bias

				for (int i = 0; i < AccumulatorLength; i += 2)
				{
					__m128i acc_element = _mm_set1_epi32(reinterpret_cast<const int32_t*>(accumulator.data() + i)[0]);
					acc_element = relu_8x16(acc_element);
					for (int j = 0; j < MiddleSize; j++)
					{
						const __m128i tmp = _mm_load_si128((const __m128i*) (layer_2.weights() + i * MiddleNeurons + j * 8)); // load weights
						acc[j] = _mm_add_epi32(acc[j], _mm_madd_epi16(acc_element, tmp));
					}
				}

				for (int j = 0; j < MiddleSize; j++)
					acc[j] = relu_4x32(acc[j]);

				__m128 out = _mm_setzero_ps();
				for (int j = 0; j < MiddleSize; j++)
				{
					const __m128 tmp = _mm_cvtepi32_ps(acc[j]);
					const __m128 w = _mm_load_ps(layer_3.weights() + j * 4);
					out = _mm_add_ps(_mm_mul_ps(tmp, w), out);
				}

				return sigmoid(layer_3.bias()[0] + horizontal_add(out));
			}
	};

}

namespace ag
{
	namespace nnue
	{

		void sse41_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
				const std::vector<int> &active) noexcept
		{
			if (accumulator.size() == 32)
				RefreshAccumulator<32>().run(layer_0, accumulator, active);
			if (accumulator.size() == 64)
				RefreshAccumulator<64>().run(layer_0, accumulator, active);
		}

		void sse41_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
				const std::vector<int> &added) noexcept
		{
			if (accumulator.size() == 32)
				UpdateAccumulator<32>().run(layer_0, accumulator, removed, added);
			if (accumulator.size() == 64)
				UpdateAccumulator<64>().run(layer_0, accumulator, removed, added);
		}

		float sse41_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
				const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept
		{
			if (layer_1.neurons() == 8)
			{
				if (accumulator.size() == 32)
					return Forward<32, 8>().run(accumulator, layer_1, fp32_layers[0]);
				if (accumulator.size() == 64)
					return Forward<64, 8>().run(accumulator, layer_1, fp32_layers[0]);
			}
			if (layer_1.neurons() == 16)
			{
				if (accumulator.size() == 32)
					return Forward<32, 16>().run(accumulator, layer_1, fp32_layers[0]);
				if (accumulator.size() == 64)
					return Forward<64, 16>().run(accumulator, layer_1, fp32_layers[0]);
			}
			return 0.0f;
		}

	} /* namespace nnue */
} /* namespace ag */
