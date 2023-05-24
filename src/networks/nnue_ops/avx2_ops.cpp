/*
 * avx2_ops.cpp
 *
 *  Created on: Oct 20, 2022
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

	__m256i relu_16x16(__m256i x) noexcept
	{
		return _mm256_max_epi16(_mm256_setzero_si256(), x);
	}
	__m256i relu_8x32(__m256i x) noexcept
	{
		return _mm256_max_epi32(_mm256_setzero_si256(), x);
	}
	float horizontal_add(__m256 x) noexcept
	{
		const __m128 x128 = _mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x));
		const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));
		const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
		return _mm_cvtss_f32(x32);
	}

	template<int AccumulatorLength>
	struct RefreshAccumulator
	{
			static_assert(AccumulatorLength % 16 == 0);

			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &active) const noexcept
			{
				assert(layer_1.neurons() == AccumulatorLength);
				assert(accumulator.size() == AccumulatorLength);

				constexpr int length = AccumulatorLength / 16;
				__m256i acc[length]; // length x (16 x int16)
				for (int j = 0; j < length; j++)
					acc[j] = _mm256_load_si256((const __m256i*) (layer_1.bias() + j * 16));
				for (size_t i = 0; i < active.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + active[i] * AccumulatorLength;
					for (int j = 0; j < length; j++, ptr += 16)
					{
						const __m128i row = _mm_load_si128((__m128i*) ptr); // loading 16 x int8
						const __m256i a = _mm256_cvtepi8_epi16(row); // unpacked into 16 x int16
						acc[j] = _mm256_add_epi16(acc[j], a);
					}
				}
				for (int j = 0; j < length; j++)
					_mm256_store_si256((__m256i*) (accumulator.data() + j * 16), acc[j]);
			}
	};
	template<>
	struct RefreshAccumulator<64>
	{
			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &active) const noexcept
			{
				assert(layer_1.neurons() == 64);
				assert(accumulator.size() == 64);

				__m256i acc0 = _mm256_load_si256((const __m256i*) (layer_1.bias() + 0 * 16));
				__m256i acc1 = _mm256_load_si256((const __m256i*) (layer_1.bias() + 1 * 16));
				__m256i acc2 = _mm256_load_si256((const __m256i*) (layer_1.bias() + 2 * 16));
				__m256i acc3 = _mm256_load_si256((const __m256i*) (layer_1.bias() + 3 * 16));
				for (size_t i = 0; i < active.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + active[i] * 64;

					const __m128i row0 = _mm_load_si128((const __m128i*) (ptr + 0 * 16)); // loading 16 x int8
					const __m128i row1 = _mm_load_si128((const __m128i*) (ptr + 1 * 16)); // loading 16 x int8
					const __m128i row2 = _mm_load_si128((const __m128i*) (ptr + 2 * 16)); // loading 16 x int8
					const __m128i row3 = _mm_load_si128((const __m128i*) (ptr + 3 * 16)); // loading 16 x int8
					acc0 = _mm256_add_epi16(acc0, _mm256_cvtepi8_epi16(row0));
					acc1 = _mm256_add_epi16(acc1, _mm256_cvtepi8_epi16(row1));
					acc2 = _mm256_add_epi16(acc2, _mm256_cvtepi8_epi16(row2));
					acc3 = _mm256_add_epi16(acc3, _mm256_cvtepi8_epi16(row3));
				}
				_mm256_store_si256((__m256i*) (accumulator.data() + 0 * 16), acc0);
				_mm256_store_si256((__m256i*) (accumulator.data() + 1 * 16), acc1);
				_mm256_store_si256((__m256i*) (accumulator.data() + 2 * 16), acc2);
				_mm256_store_si256((__m256i*) (accumulator.data() + 3 * 16), acc3);
			}
	};

	template<int AccumulatorLength>
	struct UpdateAccumulator
	{
			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
					const std::vector<int> &added) const noexcept
			{
				assert(layer_1.neurons() == AccumulatorLength);
				assert(accumulator.size() == AccumulatorLength);

				constexpr int Parts = AccumulatorLength / 16;
				// 2 x (16 x int16) - together 32 accumulators for difference in features
				__m256i acc[4];
				for (int i = 0; i < 4; i++)
					acc[i] = _mm256_load_si256((const __m256i*) (accumulator.data() + i * 16));
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + removed[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_sub_epi16(acc[0], a);
					acc[1] = _mm256_sub_epi16(acc[1], b);
					acc[2] = _mm256_sub_epi16(acc[2], c);
					acc[3] = _mm256_sub_epi16(acc[3], d);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + added[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_add_epi16(acc[0], a);
					acc[1] = _mm256_add_epi16(acc[1], b);
					acc[2] = _mm256_add_epi16(acc[2], c);
					acc[3] = _mm256_add_epi16(acc[3], d);
				}

				for (int i = 0; i < 4; i++)
					_mm256_storeu_si256((__m256i*) (accumulator.data() + i * 16), acc[i]);
			}
	};
	template<>
	struct UpdateAccumulator<32>
	{
			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
					const std::vector<int> &added) const noexcept
			{
				assert(layer_1.neurons() == 32);
				assert(accumulator.size() == 32);

				// 2 x (16 x int16) - together 32 accumulators for difference in features
				__m256i acc[2];
				for (int i = 0; i < 2; i++)
					acc[i] = _mm256_load_si256((const __m256i*) (accumulator.data() + i * 16));
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + removed[i] * 32;
					const __m256i row = _mm256_loadu_si256((const __m256i*) ptr); // loading entire row (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_sub_epi16(acc[0], a);
					acc[1] = _mm256_sub_epi16(acc[1], b);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + added[i] * 32;
					const __m256i row = _mm256_loadu_si256((const __m256i*) ptr); // loading entire row (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_add_epi16(acc[0], a);
					acc[1] = _mm256_add_epi16(acc[1], b);
				}

				for (int i = 0; i < 2; i++)
					_mm256_storeu_si256((__m256i*) (accumulator.data() + i * 16), acc[i]);
			}
	};
	template<>
	struct UpdateAccumulator<64>
	{
			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
					const std::vector<int> &added) const noexcept
			{
				assert(layer_1.neurons() == 64);
				assert(accumulator.size() == 64);

				// 2 x (16 x int16) - together 32 accumulators for difference in features
				__m256i acc[4];
				for (int i = 0; i < 4; i++)
					acc[i] = _mm256_load_si256((const __m256i*) (accumulator.data() + i * 16));
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + removed[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_sub_epi16(acc[0], a);
					acc[1] = _mm256_sub_epi16(acc[1], b);
					acc[2] = _mm256_sub_epi16(acc[2], c);
					acc[3] = _mm256_sub_epi16(acc[3], d);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + added[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_add_epi16(acc[0], a);
					acc[1] = _mm256_add_epi16(acc[1], b);
					acc[2] = _mm256_add_epi16(acc[2], c);
					acc[3] = _mm256_add_epi16(acc[3], d);
				}

				for (int i = 0; i < 4; i++)
					_mm256_storeu_si256((__m256i*) (accumulator.data() + i * 16), acc[i]);
			}
	};
	template<>
	struct UpdateAccumulator<256>
	{
			void run(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
					const std::vector<int> &added) const noexcept
			{
				assert(layer_1.neurons() == 64);
				assert(accumulator.size() == 64);

				// 2 x (16 x int16) - together 32 accumulators for difference in features
				__m256i acc[4];
				for (int i = 0; i < 4; i++)
					acc[i] = _mm256_load_si256((const __m256i*) (accumulator.data() + i * 16));
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + removed[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_sub_epi16(acc[0], a);
					acc[1] = _mm256_sub_epi16(acc[1], b);
					acc[2] = _mm256_sub_epi16(acc[2], c);
					acc[3] = _mm256_sub_epi16(acc[3], d);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights() + added[i] * 64;
					const __m256i row0 = _mm256_loadu_si256((const __m256i*) (ptr + 0 * 32)); // loading first half (32 x int8)
					const __m256i row1 = _mm256_loadu_si256((const __m256i*) (ptr + 1 * 32)); // loading second half (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row0)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row0, 1)); // unpacked second half into 16 x int16
					const __m256i c = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row1)); // unpacked first half into 16 x int16
					const __m256i d = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row1, 1)); // unpacked second half into 16 x int16
					acc[0] = _mm256_add_epi16(acc[0], a);
					acc[1] = _mm256_add_epi16(acc[1], b);
					acc[2] = _mm256_add_epi16(acc[2], c);
					acc[3] = _mm256_add_epi16(acc[3], d);
				}

				for (int i = 0; i < 4; i++)
					_mm256_storeu_si256((__m256i*) (accumulator.data() + i * 16), acc[i]);
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

				constexpr int MiddleSize = MiddleNeurons / 8;

				__m256i acc[MiddleSize];
				for (int j = 0; j < MiddleSize; j++)
					acc[j] = _mm256_load_si256((const __m256i*) (layer_2.bias() + j * 8)); // load bias

				for (int i = 0; i < AccumulatorLength; i += 2)
				{
					__m256i acc_element = _mm256_castps_si256(_mm256_broadcast_ss((reinterpret_cast<const float*>(accumulator.data() + i))));
					acc_element = relu_16x16(acc_element);

//					if (not _mm256_testz_si256(acc_element, acc_element))
					for (int j = 0; j < MiddleSize; j++)
					{
						__m256i tmp = _mm256_load_si256((const __m256i*) (layer_2.weights() + i * MiddleNeurons + j * 16)); // load weights
						acc[j] = _mm256_add_epi32(acc[j], _mm256_madd_epi16(acc_element, tmp));
					}
				}

				for (int j = 0; j < MiddleSize; j++)
					acc[j] = relu_8x32(acc[j]);

				__m256 out = _mm256_setzero_ps();
				for (int j = 0; j < MiddleSize; j++)
				{
					const __m256 tmp = _mm256_cvtepi32_ps(acc[j]);
					const __m256 w = _mm256_load_ps(layer_3.weights() + j * 8);
					out = _mm256_fmadd_ps(tmp, w, out);
				}

				return sigmoid(layer_3.bias()[0] + horizontal_add(out));
			}
	};

	template<>
	struct Forward<64, 16>
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

				__m256i acc0 = _mm256_load_si256((const __m256i*) (layer_2.bias() + 0 * 8)); // load bias
				__m256i acc1 = _mm256_load_si256((const __m256i*) (layer_2.bias() + 1 * 8)); // load bias

				for (int i = 0; i < 64; i += 2)
				{
					__m256i acc_element = _mm256_castps_si256(_mm256_broadcast_ss((reinterpret_cast<const float*>(accumulator.data() + i))));
					acc_element = relu_16x16(acc_element);

//					if (not _mm256_testz_si256(acc_element, acc_element))
					const __m256i tmp0 = _mm256_load_si256((const __m256i*) (layer_2.weights() + i * 16 + 0 * 16)); // load weights
					const __m256i tmp1 = _mm256_load_si256((const __m256i*) (layer_2.weights() + i * 16 + 1 * 16)); // load weights
					acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(acc_element, tmp0));
					acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(acc_element, tmp1));
				}

				acc0 = relu_8x32(acc0);
				acc1 = relu_8x32(acc1);

				const __m256 tmp0 = _mm256_cvtepi32_ps(acc0);
				const __m256 tmp1 = _mm256_cvtepi32_ps(acc1);

				__m256 w0 = _mm256_load_ps(layer_3.weights() + 0 * 8);
				__m256 w1 = _mm256_load_ps(layer_3.weights() + 1 * 8);
				w0 = _mm256_mul_ps(w0, tmp0);
				w1 = _mm256_mul_ps(w1, tmp1);

				return sigmoid(layer_3.bias()[0] + horizontal_add(_mm256_add_ps(w0, w1)));
			}
	};

}

namespace ag
{
	namespace nnue
	{

		void avx2_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
				const std::vector<int> &active) noexcept
		{
			if (accumulator.size() == 32)
				RefreshAccumulator<32>().run(layer_0, accumulator, active);
			if (accumulator.size() == 64)
				RefreshAccumulator<64>().run(layer_0, accumulator, active);
		}

		void avx2_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
				const std::vector<int> &added) noexcept
		{
			if (accumulator.size() == 32)
				UpdateAccumulator<32>().run(layer_0, accumulator, removed, added);
			if (accumulator.size() == 64)
				UpdateAccumulator<64>().run(layer_0, accumulator, removed, added);
		}

		float avx2_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
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
			if (layer_1.neurons() == 32 and accumulator.size() == 256)
				return Forward<256, 32>().run(accumulator, layer_1, fp32_layers[0]);

			return 0.0f;
		}

	} /* namespace nnue */
} /* namespace ag */
