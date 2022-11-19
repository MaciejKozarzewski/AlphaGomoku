/*
 * avx2_ops.cpp
 *
 *  Created on: Oct 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/avx2_ops.hpp>

#include <x86intrin.h>
#include <cassert>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

	__m128 relu(__m128 x) noexcept
	{
		return _mm_max_ps(_mm_setzero_ps(), x);
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
			void run(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) const noexcept
			{
				assert(AccumulatorLength % 8 == 0);
				assert(layer_1.weights.cols() == AccumulatorLength);
				assert(accumulator.size() == AccumulatorLength);

				constexpr int length = AccumulatorLength / 8;
				__m256i acc[length]; // length x (8 x int32)
				for (int j = 0; j < length; j++)
					acc[j] = _mm256_loadu_si256((const __m256i*) (layer_1.bias.data() + j * 8));
				for (size_t i = 0; i < active.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + active[i] * AccumulatorLength;
					for (int j = 0; j < length; j++, ptr += 8)
					{
						const __m128i row = _mm_loadu_si64(ptr); // loading 8 x int8
						const __m256i a = _mm256_cvtepi8_epi32(row); // unpacked into 8 x int32
						acc[j] = _mm256_add_epi32(acc[j], a);
					}
				}
				for (int j = 0; j < length; j++)
					_mm256_storeu_si256((__m256i*) (accumulator.data() + j * 8), acc[j]);
			}
	};

	template<int AccumulatorLength>
	struct UpdateAccumulator
	{
			void run(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
					const std::vector<int> &removed, const std::vector<int> &added) const noexcept
			{
			}
	};
	template<>
	struct UpdateAccumulator<32>
	{
			void run(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
					const std::vector<int> &removed, const std::vector<int> &added) const noexcept
			{
				assert(layer_1.weights.cols() == 32);
				assert(oldAccumulator.size() == 32);
				assert(newAccumulator.size() == 32);

				// 2 x (16 x int16) - together 32 accumulators for difference in features
				__m256i diff[2] = { _mm256_setzero_si256(), _mm256_setzero_si256() };
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + removed[i] * 32;
					const __m256i row = _mm256_loadu_si256((const __m256i*) ptr); // loading entire row (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					diff[0] = _mm256_sub_epi16(diff[0], a);
					diff[1] = _mm256_sub_epi16(diff[1], b);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + added[i] * 32;
					const __m256i row = _mm256_loadu_si256((const __m256i*) ptr); // loading entire row (32 x int8)
					const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
					const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					diff[0] = _mm256_add_epi16(diff[0], a);
					diff[1] = _mm256_add_epi16(diff[1], b);
				}

				// load old accumulator
				__m256i old_acc[4];
				for (int i = 0; i < 4; i++)
					old_acc[i] = _mm256_loadu_si256((const __m256i*) (oldAccumulator.data() + i * 8));
				// unpack the difference from int16 into int32
				__m256i new_acc[4];
				for (int i = 0; i < 2; i++)
				{
					new_acc[2 * i + 0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(diff[i]));
					new_acc[2 * i + 1] = _mm256_cvtepi16_epi32(_mm256_extractf128_si256(diff[i], 1));
				}
				// add the difference to the old accumulator
				for (int i = 0; i < 4; i++)
					old_acc[i] = _mm256_add_epi32(old_acc[i], new_acc[i]);
				// store into new accumulator
				for (int i = 0; i < 4; i++)
					_mm256_storeu_si256((__m256i*) (newAccumulator.data() + i * 8), old_acc[i]);
			}
	};
	template<>
	struct UpdateAccumulator<64>
	{
			void run(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
					const std::vector<int> &removed, const std::vector<int> &added) const noexcept
			{
				assert(layer_1.weights.cols() == 64);
				assert(oldAccumulator.size() == 64);
				assert(newAccumulator.size() == 64);

				// 4 x (16 x int16) - together 64 accumulators for difference in features
				__m256i diff[4] = { _mm256_setzero_si256(), _mm256_setzero_si256(), _mm256_setzero_si256(), _mm256_setzero_si256() };
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + removed[i] * 64;
					__m256i tmp[4];
					for (int j = 0; j < 2; j++)
					{
						const __m256i row = _mm256_loadu_si256((const __m256i*) (ptr + j * 32)); // loading half of a row (32 x int8)
						tmp[2 * j + 0] = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
						tmp[2 * j + 1] = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					}
					for (int j = 0; j < 4; j++)
						diff[j] = _mm256_sub_epi16(diff[j], tmp[j]);
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + added[i] * 64;
					__m256i tmp[4];
					for (int j = 0; j < 2; j++)
					{
						const __m256i row = _mm256_loadu_si256((const __m256i*) (ptr + j * 32)); // loading half of a row (32 x int8)
						tmp[2 * j + 0] = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
						tmp[2 * j + 1] = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
					}
					for (int j = 0; j < 4; j++)
						diff[j] = _mm256_add_epi16(diff[j], tmp[j]);
				}

				// load old accumulator
				__m256i old_acc[8];
				for (int i = 0; i < 8; i++)
					old_acc[i] = _mm256_loadu_si256((const __m256i*) (oldAccumulator.data() + i * 8));
				// unpack the difference from int16 into int32
				__m256i new_acc[8];
				for (int i = 0; i < 4; i++)
				{
					new_acc[2 * i + 0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(diff[i]));
					new_acc[2 * i + 1] = _mm256_cvtepi16_epi32(_mm256_extractf128_si256(diff[i], 1));
				}
				// add the difference to the old accumulator
				for (int i = 0; i < 8; i++)
					old_acc[i] = _mm256_add_epi32(old_acc[i], new_acc[i]);
				// store into new accumulator
				for (int i = 0; i < 8; i++)
					_mm256_storeu_si256((__m256i*) (newAccumulator.data() + i * 8), old_acc[i]);
			}
	};

	template<int AccumulatorLength, int MiddleNeurons>
	struct Forward
	{
			float run(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
					const RealSpaceLayer &layer_3) const noexcept
			{
				return 0.0f;
			}
	};
	template<int AccumulatorLength>
	struct Forward<AccumulatorLength, 8>
	{
			float run(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
					const RealSpaceLayer &layer_3) const noexcept
			{
				__m256 middle0 = _mm256_loadu_ps(layer_2.bias.data());
				__m256 middle1 = _mm256_setzero_ps();
				__m256 middle2 = _mm256_setzero_ps();
				__m256 middle3 = _mm256_setzero_ps();
				const float *ptr = layer_2.weights.data();
				for (int i = 0; i < AccumulatorLength; i += 4, ptr += 4 * 8)
				{
					// load 4 elements of the accumulator as int32 and convert it to fp32
					const __m128 acc = _mm_cvtepi32_ps(_mm_loadu_si128((const __m128i*) (accumulator.data() + i)));
					// load 4 elements of scale factors
					const __m128 scale = _mm_loadu_ps(layer_1.scale.data() + i);
					// apply scaling and then activation function of the first layer
					const __m128 input = relu(_mm_mul_ps(acc, scale));

					const __m256 inp = _mm256_set_m128(input, input);
					const __m256 tmp0 = _mm256_shuffle_ps(inp, inp, 0x00);
					const __m256 tmp1 = _mm256_shuffle_ps(inp, inp, 0x55);
					const __m256 tmp2 = _mm256_shuffle_ps(inp, inp, 0xaa);
					const __m256 tmp3 = _mm256_shuffle_ps(inp, inp, 0xff);

					middle0 = _mm256_fmadd_ps(tmp0, _mm256_loadu_ps(ptr + 0), middle0);
					middle1 = _mm256_fmadd_ps(tmp1, _mm256_loadu_ps(ptr + 8), middle1);
					middle2 = _mm256_fmadd_ps(tmp2, _mm256_loadu_ps(ptr + 16), middle2);
					middle3 = _mm256_fmadd_ps(tmp3, _mm256_loadu_ps(ptr + 24), middle3);
				}
				middle0 = _mm256_add_ps(middle0, middle1);
				middle2 = _mm256_add_ps(middle2, middle3);
				middle0 = _mm256_add_ps(middle0, middle2);
				middle0 = _mm256_max_ps(_mm256_setzero_ps(), middle0); // relu

				const __m256 mat = _mm256_loadu_ps(layer_3.weights.data());
				const __m256 tmp = _mm256_mul_ps(mat, middle0);
				const float output = layer_3.bias[0] + horizontal_add(tmp);
				return approximate_sigmoid(output);
			}
	};
}

namespace ag
{
	namespace nnue
	{

		void avx2_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
		{
			if (accumulator.size() == 32)
				RefreshAccumulator<32>().run(layer_1, accumulator, active);
			if (accumulator.size() == 64)
				RefreshAccumulator<64>().run(layer_1, accumulator, active);
		}

		void avx2_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
				const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
			if (oldAccumulator.size() == 32)
				UpdateAccumulator<32>().run(layer_1, oldAccumulator, newAccumulator, removed, added);
			if (oldAccumulator.size() == 64)
				UpdateAccumulator<64>().run(layer_1, oldAccumulator, newAccumulator, removed, added);
		}

		float avx2_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
				const RealSpaceLayer &layer_3) noexcept
		{
			if (layer_2.bias.size() == 8)
			{
				if (accumulator.size() == 32)
					return Forward<32, 8>().run(accumulator, layer_1, layer_2, layer_3);
				if (accumulator.size() == 64)
					return Forward<64, 8>().run(accumulator, layer_1, layer_2, layer_3);
			}
			return 0.0f;
		}

	} /* namespace nnue */
} /* namespace ag */
