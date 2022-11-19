/*
 * sse2_ops.cpp
 *
 *  Created on: Oct 23, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/sse2_ops.hpp>

#include <x86intrin.h>
#include <cassert>
#include <iostream>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

	__m128 relu(__m128 x) noexcept
	{
		return _mm_max_ps(_mm_setzero_ps(), x);
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
			void run(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) const noexcept
			{
				assert(layer_1.weights.cols() % 8 == 0);
				assert(accumulator.size() % 8 == 0);

				for (int j = 0; j < AccumulatorLength; j++)
					accumulator[j] = layer_1.bias[j];

				for (size_t i = 0; i < active.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + active[i] * AccumulatorLength;
					for (int j = 0; j < AccumulatorLength; j++)
						accumulator[j] += static_cast<int32_t>(ptr[j]);
				}
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

				// 8 x (8 x int16) - together 64 accumulators for difference in features
				__m128i diff[4];
				for (int i = 0; i < 4; i++)
					diff[i] = _mm_setzero_si128();
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + removed[i] * 32;
					for (int j = 0; j < 2; j++, ptr += 16)
					{
						__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading part of weight matrix row (16 x int8)
						const __m128i mask = _mm_cmplt_epi8(tmp, _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
						__m128i a = _mm_unpacklo_epi8(tmp, mask); // unpacked first 8 x int16
						__m128i b = _mm_unpackhi_epi8(tmp, mask); // unpacked second 8 x int16
						diff[j * 2 + 0] = _mm_sub_epi16(diff[j * 2 + 0], a);
						diff[j * 2 + 1] = _mm_sub_epi16(diff[j * 2 + 1], b);
					}
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + added[i] * 32;
					for (int j = 0; j < 2; j++, ptr += 16)
					{
						__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading part of weight matrix row (16 x int8)
						const __m128i mask = _mm_cmplt_epi8(tmp, _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
						__m128i a = _mm_unpacklo_epi8(tmp, mask); // unpacked first 8 x int16
						__m128i b = _mm_unpackhi_epi8(tmp, mask); // unpacked second 8 x int16
						diff[j * 2 + 0] = _mm_add_epi16(diff[j * 2 + 0], a);
						diff[j * 2 + 1] = _mm_add_epi16(diff[j * 2 + 1], b);
					}
				}
				// now apply the difference to the new accumulator
				for (int i = 0; i < 4; i++)
				{
					const __m128i mask = _mm_cmplt_epi16(diff[i], _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
					__m128i a = _mm_unpacklo_epi16(diff[i], mask); // unpacked first half (4 x int16) into 4 x int32
					__m128i b = _mm_unpackhi_epi16(diff[i], mask); // unpacked second half (4 x int16) into 4 x int32
					__m128i acc0 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 0)); // load 4 x int32 of the old accumulator
					__m128i acc1 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 4));
					acc0 = _mm_add_epi32(acc0, a); // add the difference to the accumulator
					acc1 = _mm_add_epi32(acc1, b);
					_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8 + 0), acc0); // store new accumulator value
					_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8 + 4), acc1);
				}
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

				// 8 x (8 x int16) - together 64 accumulators for difference in features
				__m128i diff[8];
				for (int i = 0; i < 8; i++)
					diff[i] = _mm_setzero_si128();
				// accumulate difference between accumulators in int16
				for (size_t i = 0; i < removed.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + removed[i] * 64;
					for (int j = 0; j < 4; j++, ptr += 16)
					{
						__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading part of weight matrix row (16 x int8)
						const __m128i mask = _mm_cmplt_epi8(tmp, _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
						__m128i a = _mm_unpacklo_epi8(tmp, mask); // unpacked first 8 x int16
						__m128i b = _mm_unpackhi_epi8(tmp, mask); // unpacked second 8 x int16
						diff[j * 2 + 0] = _mm_sub_epi16(diff[j * 2 + 0], a);
						diff[j * 2 + 1] = _mm_sub_epi16(diff[j * 2 + 1], b);
					}
				}

				for (size_t i = 0; i < added.size(); i++)
				{
					const int8_t *ptr = layer_1.weights.data() + added[i] * 64;
					for (int j = 0; j < 4; j++, ptr += 16)
					{
						__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading part of weight matrix row (16 x int8)
						const __m128i mask = _mm_cmplt_epi8(tmp, _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
						__m128i a = _mm_unpacklo_epi8(tmp, mask); // unpacked first 8 x int16
						__m128i b = _mm_unpackhi_epi8(tmp, mask); // unpacked second 8 x int16
						diff[j * 2 + 0] = _mm_add_epi16(diff[j * 2 + 0], a);
						diff[j * 2 + 1] = _mm_add_epi16(diff[j * 2 + 1], b);
					}
				}

				// now apply the difference to the new accumulator
				for (int i = 0; i < 8; i++)
				{
					const __m128i mask = _mm_cmplt_epi16(diff[i], _mm_setzero_si128()); // mask of ones for negative values to be used for sign extending in the next few lines
					__m128i a = _mm_unpacklo_epi16(diff[i], mask); // unpacked first half (4 x int16) into 4 x int32
					__m128i b = _mm_unpackhi_epi16(diff[i], mask); // unpacked second half (4 x int16) into 4 x int32
					__m128i acc0 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 0)); // load 4 x int32 of the old accumulator
					__m128i acc1 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 4));
					acc0 = _mm_add_epi32(acc0, a); // add the difference to the accumulator
					acc1 = _mm_add_epi32(acc1, b);
					_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8 + 0), acc0); // store new accumulator value
					_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8 + 4), acc1);
				}
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
				// two accumulators for the output of the middle layer neurons
				__m128 middle0 = _mm_loadu_ps(layer_2.bias.data() + 0);
				__m128 middle1 = _mm_loadu_ps(layer_2.bias.data() + 4);

				const float *ptr = layer_2.weights.data();
				for (int i = 0; i < AccumulatorLength; i += 4)
				{
					// load 4 elements of the accumulator as int32 and convert it to fp32
					const __m128 acc = _mm_cvtepi32_ps(_mm_loadu_si128((const __m128i*) (accumulator.data() + i)));
					// load 4 elements of scale factors
					const __m128 scale = _mm_loadu_ps(layer_1.scale.data() + i);
					// apply scaling and then activation function of the first layer
					const __m128 input = relu(_mm_mul_ps(acc, scale));

					__m128 tmp = _mm_shuffle_ps(input, input, 0x00); // broadcast first element
					middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
					middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
					ptr += 8;

					tmp = _mm_shuffle_ps(input, input, 0x55); // broadcast second element
					middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
					middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
					ptr += 8;

					tmp = _mm_shuffle_ps(input, input, 0xaa); // broadcast third element
					middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
					middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
					ptr += 8;

					tmp = _mm_shuffle_ps(input, input, 0xff); // broadcast fourth element
					middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
					middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
					ptr += 8;
				}
				// apply layer_2 activation function
				middle0 = relu(middle0);
				middle1 = relu(middle1);
				// load weights of the output layer
				const __m128 mat0 = _mm_loadu_ps(layer_3.weights.data() + 0);
				const __m128 mat1 = _mm_loadu_ps(layer_3.weights.data() + 4);
				// calculate dot product
				middle0 = _mm_mul_ps(middle0, mat0);
				middle1 = _mm_mul_ps(middle1, mat1);
				// calculate final output
				const float output = layer_3.bias[0] + horizontal_add(_mm_add_ps(middle0, middle1));
				return approximate_sigmoid(output);
			}
	};
}

namespace ag
{
	namespace nnue
	{

		void sse2_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
		{
			if (accumulator.size() == 32)
				RefreshAccumulator<32>().run(layer_1, accumulator, active);
			if (accumulator.size() == 64)
				RefreshAccumulator<64>().run(layer_1, accumulator, active);
		}

		void sse2_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
				const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
			if (oldAccumulator.size() == 32)
				UpdateAccumulator<32>().run(layer_1, oldAccumulator, newAccumulator, removed, added);
			if (oldAccumulator.size() == 64)
				UpdateAccumulator<64>().run(layer_1, oldAccumulator, newAccumulator, removed, added);
		}

		float sse2_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
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

