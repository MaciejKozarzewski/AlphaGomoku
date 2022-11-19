/*
 * avx_ops.cpp
 *
 *  Created on: Oct 23, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/avx_ops.hpp>

#include <x86intrin.h>
#include <cassert>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

//	__m128 relu(__m128 x) noexcept
//	{
//		return _mm_max_ps(_mm_setzero_ps(), x);
//	}
//	float horizontal_add(__m128 x) noexcept
//	{
//		const __m128 x64 = _mm_add_ps(x, _mm_movehl_ps(x, x));
//		const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
//		return _mm_cvtss_f32(x32);
//	}
//
//	template<int AccumulatorLength>
//	void kernel_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
//	{
//	}
//	template<>
//	void kernel_refresh_accumulator<32>(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
//	{
//		assert(layer_1.weights.cols() == AccumulatorLength);
//		assert(accumulator.size() == AccumulatorLength);
//
//		for (int j = 0; j < 32; j++)
//			accumulator[j] = layer_1.bias[j];
//
//		for (size_t i = 0; i < active.size(); i++)
//		{
//			const int8_t *ptr = layer_1.weights.data() + active[i] * 32;
//			for (int j = 0; j < 32; j++)
//				accumulator[j] += static_cast<int32_t>(ptr[j]);
//		}
//	}
//
//	template<int AccumulatorLength>
//	void kernel_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
//			const std::vector<int> &removed, const std::vector<int> &added) noexcept
//	{
//	}
//	template<>
//	void kernel_update_accumulator<32>(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
//			const std::vector<int> &removed, const std::vector<int> &added) noexcept
//	{
//		assert(layer_1.weights.cols() == AccumulatorLength);
//		assert(oldAccumulator.size() == AccumulatorLength);
//		assert(newAccumulator.size() == AccumulatorLength);
//
//		// 4 x (8 x int16) - together 32 accumulators for difference in features
//		__m128i diff[4] = { _mm_setzero_si128(), _mm_setzero_si128(), _mm_setzero_si128(), _mm_setzero_si128() };
//		// accumulate difference between accumulators in int16
//		for (size_t i = 0; i < removed.size(); i++)
//		{
//			const int8_t *ptr = layer_1.weights.data() + removed[i] * 32;
//			__m128i row0 = _mm_loadu_si128((__m128i*) ptr); // loading first half of weight matrix row (16 x int8)
//			__m128i row1 = _mm_loadu_si128((__m128i*) (ptr + 16)); // loading second half of weight matrix row (16 x int8)
//			const __m256i row = _mm256_loadu_si256((const __m256i*) ptr); // loading entire row (32 x int8)
//			const __m256i a = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(row)); // unpacked first half into 16 x int16
//			const __m256i b = _mm256_cvtepi8_epi16(_mm256_extractf128_si256(row, 1)); // unpacked second half into 16 x int16
//			diff[0] = _mm256_sub_epi16(diff[0], a);
//			diff[1] = _mm256_sub_epi16(diff[1], b);
//		}
//		for (size_t i = 0; i < removed.size(); i++)
//		{
//			const int8_t *ptr = layer_1.weights.data() + removed[i] * 32;
//			__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading first half of weight matrix row (16 x int8)
//			__m128i a = _mm_unpacklo_epi8(_mm_setzero_si128(), tmp); // unpacked first 8 x int16
//			__m128i b = _mm_unpackhi_epi8(_mm_setzero_si128(), tmp); // unpacked second 8 x int16
//			diff[0] = _mm_sub_epi16(diff[0], a);
//			diff[1] = _mm_sub_epi16(diff[1], b);
//			tmp = _mm_loadu_si128((__m128i*) (ptr + 16)); // loading second half of weight matrix row (16 x int8)
//			a = _mm_unpacklo_epi8(_mm_setzero_si128(), tmp); // unpacked third 8 x int16
//			b = _mm_unpackhi_epi8(_mm_setzero_si128(), tmp); // unpacked fourth 8 x int16
//			diff[2] = _mm_sub_epi16(diff[2], a);
//			diff[3] = _mm_sub_epi16(diff[3], b);
//		}
//
//		for (size_t i = 0; i < added.size(); i++)
//		{
//			const int8_t *ptr = layer_1.weights.data() + added[i] * 32;
//			__m128i tmp = _mm_loadu_si128((__m128i*) ptr); // loading first half of weight matrix row (16 x int8)
//			__m128i a = _mm_unpacklo_epi8(_mm_setzero_si128(), tmp); // unpacked first 8 x int8 into 8 x int16
//			__m128i b = _mm_unpackhi_epi8(_mm_setzero_si128(), tmp); // unpacked second 8 x int8 into 8 x int16
//			diff[0] = _mm_add_epi16(diff[0], a);
//			diff[1] = _mm_add_epi16(diff[1], b);
//			tmp = _mm_loadu_si128((__m128i*) (ptr + 16)); // loading second half of weight matrix row (16 x int8)
//			a = _mm_unpacklo_epi8(_mm_setzero_si128(), tmp); // unpacked third 8 x int8 into 8 x int16
//			b = _mm_unpackhi_epi8(_mm_setzero_si128(), tmp); // unpacked fourth 8 x int8 into 8 x int16
//			diff[2] = _mm_add_epi16(diff[2], a);
//			diff[3] = _mm_add_epi16(diff[3], b);
//		}
//
//		// now apply the difference to the new accumulator
//		for (int i = 0; i < 4; i++)
//		{
//			__m128i a = _mm_unpacklo_epi16(_mm_setzero_si128(), diff[i]); // unpacked first half (4 x int16) into 4 x int32
//			__m128i b = _mm_unpackhi_epi16(_mm_setzero_si128(), diff[i]); // unpacked second half (4 x int16) into 4 x int32
//			__m128i acc0 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 0)); // load 4 x int32 of the old accumulator
//			__m128i acc1 = _mm_loadu_si128((const __m128i*) (oldAccumulator.data() + i * 8 + 4));
//			acc0 = _mm_add_epi32(acc0, a); // add the difference to the accumulator
//			acc1 = _mm_add_epi32(acc1, b);
//			_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8), acc0); // store new accumulator value
//			_mm_storeu_si128((__m128i*) (newAccumulator.data() + i * 8 + 4), acc1);
//		}
//	}
//
//	template<int AccumulatorLength, int MiddleNeurons>
//	float kernel_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
//			const RealSpaceLayer &layer_3) noexcept
//	{
//		return 0.0f;
//	}
//	template<>
//	float kernel_forward<32, 8>(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
//			const RealSpaceLayer &layer_3) noexcept
//	{
//		// two accumulators for the output of the middle layer neurons
//		__m128 middle0 = _mm_loadu_ps(layer_2.bias.data() + 0);
//		__m128 middle1 = _mm_loadu_ps(layer_2.bias.data() + 4);
//
//		const float *ptr = layer_2.weights.data();
//		for (int i = 0; i < 32; i += 4)
//		{
//			// load 4 elements of the accumulator as int32 and convert it to fp32
//			const __m128 acc = _mm_cvtepi32_ps(_mm_loadu_si128((const __m128i*) (accumulator.data() + i)));
//			// load 4 elements of scale factors
//			const __m128 scale = _mm_loadu_ps(layer_1.scale.data() + i);
//			// apply scaling and then activation function of the first layer
//			const __m128 input = relu(_mm_mul_ps(acc, scale));
//
//			__m128 tmp = _mm_shuffle_ps(input, input, 0x00); // broadcast first element
//			middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
//			middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
//			ptr += 8;
//
//			tmp = _mm_shuffle_ps(input, input, 0x55); // broadcast second element
//			middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
//			middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
//			ptr += 8;
//
//			tmp = _mm_shuffle_ps(input, input, 0xaa); // broadcast third element
//			middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
//			middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
//			ptr += 8;
//
//			tmp = _mm_shuffle_ps(input, input, 0xff); // broadcast fourth element
//			middle0 = _mm_add_ps(middle0, _mm_mul_ps(tmp, _mm_loadu_ps(ptr)));
//			middle1 = _mm_add_ps(middle1, _mm_mul_ps(tmp, _mm_loadu_ps(ptr + 4)));
//			ptr += 8;
//		}
//		// apply layer_2 activation function
//		middle0 = relu(middle0);
//		middle1 = relu(middle1);
//		// load weights of the output layer
//		const __m128 mat0 = _mm_loadu_ps(layer_3.weights.data() + 0);
//		const __m128 mat1 = _mm_loadu_ps(layer_3.weights.data() + 4);
//		// calculate dot product
//		middle0 = _mm_mul_ps(middle0, mat0);
//		middle1 = _mm_mul_ps(middle1, mat1);
//		// calculate final output
//		const float output = layer_3.bias[0] + horizontal_add(_mm_add_ps(middle0, middle1));
//		return approximate_sigmoid(output);
//	}
}

namespace ag
{
	namespace nnue
	{

		void avx_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
		{
//			if (accumulator.size() == 32)
//				kernel_refresh_accumulator<32>(layer_1, accumulator, active);
		}

		void avx_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
				const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
//			if (oldAccumulator.size() == 32)
//				kernel_update_accumulator<32>(layer_1, oldAccumulator, newAccumulator, removed, added);
		}

		float avx_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
				const RealSpaceLayer &layer_3) noexcept
		{
//			if (accumulator.size() == 32 and layer_2.bias.size() == 8)
//				return kernel_forward<32, 8>(accumulator, layer_1, layer_2, layer_3);
			return 0.0f;
		}
	} /* namespace nnue */
} /* namespace ag */

