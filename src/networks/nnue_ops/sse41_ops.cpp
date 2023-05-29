/*
 * sse41_ops.cpp
 *
 *  Created on: Oct 23, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/nnue_ops/avx2_ops.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <x86intrin.h>
#include <cassert>
#include <iostream>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

	template<typename T>
	constexpr int register_capacity() noexcept
	{
		return 128 / (8 * sizeof(T));
	}

	template<typename T, int N>
	class SimdVector
	{
			static_assert(N % register_capacity<T>() == 0);

			static constexpr int RegCapacity = register_capacity<T>();
			static constexpr int RegCount = N / RegCapacity;
			__m128i m_storage[RegCount];
		public:
			SimdVector() noexcept = default;
			SimdVector(__m128i x) noexcept
			{
				for (int i = 0; i < reg_count(); i++)
					m_storage[i] = x;
			}
			SimdVector(const T *ptr) noexcept
			{
				load(ptr);
			}
			constexpr int reg_capacity() const noexcept
			{
				return RegCapacity;
			}
			constexpr int reg_count() const noexcept
			{
				return RegCount;
			}
			constexpr int size() const noexcept
			{
				return N;
			}
			void load(const T *ptr) noexcept
			{
				assert(ptr != nullptr);
				assert(is_aligned<__m128i >(ptr));
				for (int i = 0; i < reg_count(); i++)
					m_storage[i] = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr + i * RegCapacity));
			}
			void store(T *ptr) noexcept
			{
				assert(ptr != nullptr);
				assert(is_aligned<__m128i >(ptr));
				for (int i = 0; i < reg_count(); i++)
					_mm_store_si128(reinterpret_cast<__m128i*>(ptr + i * RegCapacity), m_storage[i]);
			}
			const __m128i& operator[](int index) const noexcept
			{
				assert(0 <= index && index < size());
				return m_storage[index];
			}
			__m128i& operator[](int index) noexcept
			{
				assert(0 <= index && index < size());
				return m_storage[index];
			}
	};
	template<int N>
	class SimdVector<float, N>
	{
			static_assert(N % register_capacity<float>() == 0);

			static constexpr int RegCapacity = register_capacity<float>();
			static constexpr int RegCount = N / RegCapacity;
			__m128 m_storage[RegCount];
		public:
			SimdVector() noexcept = default;
			SimdVector(__m128 x) noexcept
			{
				for (int i = 0; i < reg_count(); i++)
					m_storage[i] = x;
			}
			SimdVector(const float *ptr) noexcept
			{
				load(ptr);
			}
			constexpr int reg_capacity() const noexcept
			{
				return RegCapacity;
			}
			constexpr int reg_count() const noexcept
			{
				return RegCount;
			}
			constexpr int size() const noexcept
			{
				return N;
			}
			void load(const float *ptr) noexcept
			{
				assert(ptr != nullptr);
				assert(is_aligned<__m128 >(ptr));
				for (int i = 0; i < reg_count(); i++)
					m_storage[i] = _mm_load_ps(ptr + i * RegCapacity);
			}
			void store(float *ptr) noexcept
			{
				assert(ptr != nullptr);
				assert(is_aligned<__m128 >(ptr));
				for (int i = 0; i < reg_count(); i++)
					_mm_store_ps(ptr + i * RegCapacity, m_storage[i]);
			}
			const __m128& operator[](int index) const noexcept
			{
				assert(0 <= index && index < size());
				return m_storage[index];
			}
			__m128& operator[](int index) noexcept
			{
				assert(0 <= index && index < size());
				return m_storage[index];
			}
	};

	/*
	 * Conversions
	 */
	template<int N>
	SimdVector<int16_t, N> convert_INT8_to_INT16(const SimdVector<int8_t, N> &x) noexcept
	{
		SimdVector<int16_t, N> result;
		for (int i = 0; i < x.reg_count(); i++)
		{
			result[2 * i + 0] = _mm_cvtepi8_epi16(x[i]);
			const __m128 tmp0 = _mm_castsi128_ps(x[i]);
			const __m128 tmp1 = _mm_movehl_ps(tmp0, tmp0);
			result[2 * i + 1] = _mm_cvtepi8_epi16(_mm_castps_si128(tmp1));
		}
		return result;
	}
	template<int N>
	SimdVector<float, N> convert_INT32_to_FP32(const SimdVector<int32_t, N> &x) noexcept
	{
		SimdVector<float, N> result;
		for (int i = 0; i < x.reg_count(); i++)
			result[i] = _mm_cvtepi32_ps(x[i]);
		return result;
	}

	/*
	 * Broadcasting
	 */
	SimdVector<int16_t, register_capacity<int16_t>()> broadcast_2xINT16(const int16_t *ptr) noexcept
	{
		assert(ptr != nullptr);
		assert(is_aligned<int32_t>(ptr));
		const __m128 tmp =  _mm_load_ss(reinterpret_cast<const float*>(ptr));
		return SimdVector<int16_t, register_capacity<int16_t>()>(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0x00));
	}

	/*
	 * Addition and subtraction
	 */
	template<int N>
	SimdVector<int16_t, N> add(const SimdVector<int16_t, N> &a, const SimdVector<int16_t, N> &b) noexcept
	{
		SimdVector<int16_t, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_add_epi16(a[i], b[i]);
		return result;
	}
	template<int N>
	SimdVector<int16_t, N> sub(const SimdVector<int16_t, N> &a, const SimdVector<int16_t, N> &b) noexcept
	{
		SimdVector<int16_t, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_sub_epi16(a[i], b[i]);
		return result;
	}
	template<int N>
	SimdVector<float, N> add(const SimdVector<float, N> &a, const SimdVector<float, N> &b) noexcept
	{
		SimdVector<float, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_add_ps(a[i], b[i]);
		return result;
	}

	/*
	 * Multiplication and FMA
	 */
	template<int N>
	SimdVector<int32_t, N> mul_add_1xN(const SimdVector<int16_t, register_capacity<int16_t>()> &a, const SimdVector<int16_t, 2 * N> &b,
			const SimdVector<int32_t, N> &c) noexcept
	{
		assert(a.reg_count() == 1);
		assert(b.reg_count() == c.reg_count());
		SimdVector<int32_t, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_add_epi32(c[i], _mm_madd_epi16(a[0], b[i]));
		return result;
	}
	template<int N>
	SimdVector<float, N> mul_add_1xN(const SimdVector<float, register_capacity<float>()> &a, const SimdVector<float, N> &b,
			const SimdVector<float, N> &c) noexcept
	{
		assert(a.reg_count() == 1);
		SimdVector<float, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_add_ps(_mm_mul_ps(a[0], b[i]), c[i]);
		return result;
	}
	template<int N>
	SimdVector<float, N> mul(const SimdVector<float, N> &a, const SimdVector<float, N> &b) noexcept
	{
		SimdVector<float, N> result;
		for (int i = 0; i < b.reg_count(); i++)
			result[i] = _mm_mul_ps(a[i], b[i]);
		return result;
	}

	/*
	 * ReLU
	 */
	template<int N>
	SimdVector<int16_t, N> relu(const SimdVector<int16_t, N> &x) noexcept
	{
		SimdVector<int16_t, N> result;
		for (int i = 0; i < x.reg_count(); i++)
			result[i] = _mm_max_epi16(_mm_setzero_si128(), x[i]);
		return result;
	}
	template<int N>
	SimdVector<int32_t, N> relu(const SimdVector<int32_t, N> &x) noexcept
	{
		SimdVector<int32_t, N> result;
		for (int i = 0; i < x.reg_count(); i++)
			result[i] = _mm_max_epi32(_mm_setzero_si128(), x[i]);
		return result;
	}
	template<int N>
	SimdVector<float, N> relu(const SimdVector<float, N> &x) noexcept
	{
		SimdVector<float, N> result;
		for (int i = 0; i < x.reg_count(); i++)
			result[i] = _mm_max_ps(_mm_setzero_ps(), x[i]);
		return result;
	}

	/*
	 * Horizontal reduction
	 */
	template<int N>
	float horizontal_add(const SimdVector<float, N> &x) noexcept
	{
		__m128 x128 = x[0];
		for (int i = 1; i < x.reg_count(); i++)
			x128 = _mm_add_ps(x128, x[i]);
		const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));
		const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
		return _mm_cvtss_f32(x32);
	}

	template<int N>
	void update_4xN(SimdVector<float, N> &res, __m128 in, const float *weights) noexcept
	{
		const SimdVector<float, register_capacity<float>()> in0(_mm_shuffle_ps(in, in, 0x00)); // broadcast element 0
		const SimdVector<float, register_capacity<float>()> in1(_mm_shuffle_ps(in, in, 0x55)); // broadcast element 1
		const SimdVector<float, register_capacity<float>()> in2(_mm_shuffle_ps(in, in, 0xAA)); // broadcast element 2
		const SimdVector<float, register_capacity<float>()> in3(_mm_shuffle_ps(in, in, 0xFF)); // broadcast element 3

		const SimdVector<float, N> w0(weights + 0 * N);
		const SimdVector<float, N> w1(weights + 1 * N);
		const SimdVector<float, N> w2(weights + 2 * N);
		const SimdVector<float, N> w3(weights + 3 * N);

		SimdVector<float, N> tmp(_mm_setzero_ps());
		tmp = mul_add_1xN(in0, w0, tmp);
		res = mul_add_1xN(in1, w1, res);
		tmp = mul_add_1xN(in2, w2, tmp);
		res = mul_add_1xN(in3, w3, res);

		res = add(res, tmp);
	}

	template<int N>
	void refresh_accumulator_impl(const NnueLayer<int8_t, int16_t> &layer_1, Accumulator<int16_t> &accumulator,
			const std::vector<int> &active) noexcept
	{
		assert(layer_1.neurons() == N);
		assert(accumulator.size() == N);

		SimdVector<int16_t, N> acc(layer_1.bias());
		for (size_t i = 0; i < active.size(); i++)
		{
			assert(0 <= active[i] && active[i] < layer_1.inputs());
			const SimdVector<int8_t, N> weights(layer_1.weights() + active[i] * N);
			acc = add(acc, convert_INT8_to_INT16(weights));
		}
		acc.store(accumulator.data());
	}

	template<int N>
	void update_accumulator_impl(const NnueLayer<int8_t, int16_t> &layer_1, const Accumulator<int16_t> &oldAccumulator,
			Accumulator<int16_t> &newAccumulator, const std::vector<int> &removed, const std::vector<int> &added) noexcept
	{
		assert(layer_1.neurons() == N);
		assert(oldAccumulator.size() == N);
		assert(newAccumulator.size() == N);

		SimdVector<int16_t, N> acc(oldAccumulator.data());
		for (size_t i = 0; i < removed.size(); i++)
		{
			assert(0 <= removed[i] && removed[i] < layer_1.inputs());
			const SimdVector<int8_t, N> weights(layer_1.weights() + removed[i] * N);
			acc = sub(acc, convert_INT8_to_INT16(weights));
		}
		for (size_t i = 0; i < added.size(); i++)
		{
			assert(0 <= added[i] && added[i] < layer_1.inputs());
			const SimdVector<int8_t, N> weights(layer_1.weights() + added[i] * N);
			acc = add(acc, convert_INT8_to_INT16(weights));
		}
		acc.store(newAccumulator.data());

	}

	template<int Neurons>
	inline SimdVector<float, Neurons> run_int16_layer(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer) noexcept
	{
		assert(accumulator.size() % 4 == 0);
		assert(layer.inputs() == accumulator.size());
		assert(layer.neurons() == Neurons);
		/*
		 * accumulator is loaded and broadcasted as 2xint16 (a0, a1)
		 * 														/col 0 \  /col 1 \
			 * weights are loaded as columns interleaved by 2 rows (b00, b10, b01, b11, ...)
		 * 														\2 rows/  \2 rows/
		 */
		SimdVector<int32_t, Neurons> output(layer.bias());

		for (int i = 0; i < accumulator.size(); i += 4)
		{
			SimdVector<int16_t, register_capacity<int16_t>()> acc_element0 = relu(broadcast_2xINT16(accumulator.data() + i + 0)); // broadcast elements acc0, acc1
			SimdVector<int16_t, register_capacity<int16_t>()> acc_element1 = relu(broadcast_2xINT16(accumulator.data() + i + 2)); // broadcast elements acc0, acc1

			const SimdVector<int16_t, 2 * Neurons> w0(layer.weights() + (i + 0) * Neurons); // load 2 rows of weights
			const SimdVector<int16_t, 2 * Neurons> w1(layer.weights() + (i + 2) * Neurons); // load 2 rows of weights
			output = mul_add_1xN(acc_element0, w0, output);
			output = mul_add_1xN(acc_element1, w1, output);
		}
		return convert_INT32_to_FP32(relu(output));
	}
	template<int Inputs, int Neurons>
	inline SimdVector<float, Neurons> run_fp32_layer(const SimdVector<float, Inputs> &input, const NnueLayer<float, float> &layer) noexcept
	{
		assert(layer.inputs() == Inputs);
		assert(layer.neurons() == Neurons);

		SimdVector<float, Neurons> output(layer.bias()); // load bias
		for (int i = 0; i < Inputs / 4; i++)
			update_4xN(output, input[i], layer.weights() + i * 4 * Neurons); // processing 4 rows at the time
		return relu(output);
	}
	template<int Inputs>
	inline float run_final_fp32_layer(const SimdVector<float, Inputs> &input, const NnueLayer<float, float> &layer) noexcept
	{
		assert(layer.inputs() == Inputs);
		assert(layer.neurons() == 1);

		const SimdVector<float, Inputs> weights(layer.weights());
		const SimdVector<float, Inputs> output = mul(input, weights);
		return sigmoid(layer.bias()[0] + horizontal_add(output));
	}

}

namespace ag
{
	namespace nnue
	{

		void sse41_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
				const std::vector<int> &active) noexcept
		{
			refresh_accumulator_impl<64>(layer_0, accumulator, active);
		}

		void sse41_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, const Accumulator<int16_t> &oldAccumulator,
				Accumulator<int16_t> &newAccumulator, const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
			update_accumulator_impl<64>(layer_0, oldAccumulator, newAccumulator, removed, added);
		}

		float sse41_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
				const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept
		{
			const auto out2 = run_int16_layer<16>(accumulator, layer_1);
			const auto out3 = run_fp32_layer<16, 16>(out2, fp32_layers[0]);
			return run_final_fp32_layer(out3, fp32_layers[1]);
		}

	} /* namespace nnue */
} /* namespace ag */
