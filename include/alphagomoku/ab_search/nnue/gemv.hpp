/*
 * gemv.hpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_NNUE_GEMV_HPP_
#define ALPHAGOMOKU_AB_SEARCH_NNUE_GEMV_HPP_

#include <cmath>
#include <cassert>
#include <vector>
#include <x86intrin.h>

namespace ag
{
	namespace nnue
	{
		static inline float horizontal_add(__m256 x) noexcept
		{
			const __m128 x128 = _mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x));
			const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));
			const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
			return _mm_cvtss_f32(x32);
		}

		/*
		 * approximate sigmoid
		 *   y = 0.5 + x / 4 + x^2 / 32	  for   x in range [-4, 0]
		 *   y = 0.5 + x / 4 - x^2 / 32	  for   x in range [0, 4]
		 * valid from x in range [-4, 4]
		 */
		template<typename T>
		T sigmoid(T x) noexcept
		{
			return static_cast<T>(1.0) / (static_cast<T>(1.0) + std::exp(-x));
		}

		void sigmoid_forward(std::vector<float> &vector) noexcept
		{
			for (size_t i = 0; i < vector.size(); i++)
				vector[i] = sigmoid(vector[i]);
		}
		void sigmoid_backward(const std::vector<float> &vector, std::vector<float> &gradient) noexcept
		{
			assert(vector.size() == gradient.size());
			for (size_t i = 0; i < vector.size(); i++)
				gradient[i] *= vector[i] * (1.0f - vector[i]);
		}

		/*
		 * approximate tanh
		 *   y = x + x^2 / 4	  for   x in range [-4, 0]
		 *   y = x - x^2 / 4	  for   x in range [0, 4]
		 * valid from x in range [-4, 4]
		 */
		void tanh_forward(std::vector<float> &vector) noexcept
		{
			for (size_t i = 0; i < vector.size(); i++)
				vector[i] = std::tanh(vector[i]);
		}
		void tanh_backward(const std::vector<float> &vector, std::vector<float> &gradient) noexcept
		{
			assert(vector.size() == gradient.size());
			for (size_t i = 0; i < vector.size(); i++)
				gradient[i] *= (1.0f - vector[i] * vector[i]);
		}

		void relu_forward(std::vector<float> &vector) noexcept
		{
			for (size_t i = 0; i < vector.size(); i++)
				vector[i] = std::max(0.0f, vector[i]);
		}
		void relu_backward(const std::vector<float> &vector, std::vector<float> &gradient) noexcept
		{
			assert(vector.size() == gradient.size());
			for (size_t i = 0; i < vector.size(); i++)
				if (vector[i] == 0.0f)
					gradient[i] *= 0.01f; // add small amount of leakage to prevent dead neurons
		}

		void clipped_relu_forward(std::vector<float> &vector, float maxValue) noexcept
		{
			for (size_t i = 0; i < vector.size(); i++)
				vector[i] = std::max(0.0f, std::min(maxValue, vector[i]));
		}
		void clipped_relu_backward(const std::vector<float> &vector, std::vector<float> &gradient, float maxValue, float leak = 0.01f) noexcept
		{
			assert(vector.size() == gradient.size());
			for (size_t i = 0; i < vector.size(); i++)
				if (vector[i] == 0.0f or vector[i] == maxValue)
					gradient[i] *= leak;
		}

		/*
		 * approximate exp(x)
		 *   polynomial_4th(0.00665255417296, 0.082012068009070, 0.39925652526331, 0.956017570255766, 1.0)   for   x in range [-4, 0]
		 *   polynomial_4th(0.36321715083922, -1.33376721973984, 2.93502782435127, -0.24619592699161, 1.0)   for   x in range [0, 4]
		 */

		float gemv_sigmoid(const float *matrix, const float *input, const float bias, int InputLength)
		{
			assert(InputLength % 8 == 0);

#ifdef __AVX__
			__m256 acc = _mm256_setzero_ps();
			for (int j = 0; j < InputLength; j += 8)
			{
				const __m256 in = _mm256_loadu_ps(input + j);
				const __m256 mat = _mm256_loadu_ps(matrix + j);
				acc = _mm256_add_ps(acc, _mm256_mul_ps(in, mat));
			}
			float tmp = horizontal_add(acc);
#else
			float tmp = 0.0f;
			for (int j = 0; j < InputLength; j++)
				tmp += input[j] * matrix[j];
#endif
			return sigmoid(bias + tmp);
		}
		void gemTv_sigmoid(const float *matrix, float gradientNext, float *gradientPrev, float *matrixUpdate, const float *input, int InputLength)
		{
			assert(InputLength % 8 == 0);

			for (int j = 0; j < InputLength; j++)
				gradientPrev[j] = 0.0f;
#ifdef __AVX__
			const __m256 grad_next = _mm256_set1_ps(gradientNext);
			for (int j = 0; j < InputLength; j += 8)
			{
				const __m256 mat = _mm256_loadu_ps(matrix + j);
				__m256 grad_prev = _mm256_loadu_ps(gradientPrev + j);
				grad_prev = _mm256_add_ps(grad_prev, _mm256_mul_ps(grad_next, mat));
				_mm256_storeu_ps(gradientPrev + j, grad_prev);

				const __m256 in = _mm256_loadu_ps(input + j);
				__m256 mat_up = _mm256_loadu_ps(matrixUpdate + j);
				mat_up = _mm256_add_ps(mat_up, _mm256_mul_ps(grad_next, in));
				_mm256_storeu_ps(matrixUpdate + j, mat_up);
			}
#else
			for (int j = 0; j < InputLength; j++)
			{
				gradientPrev[j] += gradientNext * matrix[j];
				matrixUpdate[j] += gradientNext * input[j];
			}
#endif
		}

		void gemv_softmax(const float *matrix, const float *input, float *output, const float *bias, int InputLength)
		{
			assert(InputLength % 8 == 0);

			for (int i = 0; i < 3; i++)
			{
#ifdef __AVX__
				__m256 acc = _mm256_setzero_ps();
				for (int j = 0; j < InputLength; j += 8)
				{
					const __m256 in = _mm256_loadu_ps(input + j);
					const __m256 mat = _mm256_loadu_ps(matrix + i * InputLength + j);
					acc = _mm256_add_ps(acc, _mm256_mul_ps(in, mat));
				}
				float tmp = horizontal_add(acc);
#else
				float tmp = 0.0f;
				for (int j = 0; j < InputLength; j++)
					tmp += input[j] * matrix[i * InputLength + j];
#endif
				output[i] = bias[i] + tmp;
			}
			const float max_value = std::max(output[0], std::max(output[1], output[2]));
			for (int i = 0; i < 3; i++)
				output[i] = std::exp(output[i] - max_value);
			const float sum = 1.0f / (output[0] + output[1] + output[2]);
			for (int i = 0; i < 3; i++)
				output[i] *= sum;
		}
		void gemTv_softmax(const float *matrix, const float *gradientNext, float *gradientPrev, float *matrixUpdate, const float *input,
				int InputLength)
		{
			assert(InputLength % 8 == 0);

			for (int j = 0; j < InputLength; j++)
				gradientPrev[j] = 0.0f;
			for (int i = 0; i < 3; i++)
			{
#ifdef __AVX__
				const __m256 grad_next = _mm256_set1_ps(gradientNext[i]);
				for (int j = 0; j < InputLength; j += 8)
				{
					const __m256 mat = _mm256_loadu_ps(matrix + i * InputLength + j);
					__m256 grad_prev = _mm256_loadu_ps(gradientPrev + j);
					grad_prev = _mm256_add_ps(grad_prev, _mm256_mul_ps(grad_next, mat));
					_mm256_storeu_ps(gradientPrev + j, grad_prev);

					const __m256 in = _mm256_loadu_ps(input + j);
					__m256 mat_up = _mm256_loadu_ps(matrixUpdate + i * InputLength + j);
					mat_up = _mm256_add_ps(mat_up, _mm256_mul_ps(grad_next, in));
					_mm256_storeu_ps(matrixUpdate + i * InputLength + j, mat_up);
				}
#else
				const float grad = gradientNext[i];
				for (int j = 0; j < InputLength; j++)
				{
					gradientPrev[j] += grad * matrix[i * InputLength + j];
					matrixUpdate[i * InputLength + j] += grad * input[j];
				}
#endif
			}
		}

		void gemv(const float *matrix, const float *input, float *output, const float *bias, int MatrixColumns, int InputLength)
		{
			assert(MatrixColumns % 8 == 0);
			assert(InputLength % 8 == 0);

//			__m256 acc0 = _mm256_loadu_ps(bias);
//			__m256 acc1 = _mm256_setzero_ps();
//			__m256 acc2 = _mm256_setzero_ps();
//			__m256 acc3 = _mm256_setzero_ps();
//			for (int i = 0; i < InputLength; i += 4, matrix += 32)
//			{
//				__m128 in = _mm_loadu_ps(input + i);
//				__m256 inp = _mm256_set_m128(in, in);
//				__m256 tmp0 = _mm256_shuffle_ps(inp, inp, 0x00);
//				__m256 tmp1 = _mm256_shuffle_ps(inp, inp, 0x55);
//				__m256 tmp2 = _mm256_shuffle_ps(inp, inp, 0xaa);
//				__m256 tmp3 = _mm256_shuffle_ps(inp, inp, 0xff);
//
//				acc0 = _mm256_fmadd_ps(tmp0, _mm256_loadu_ps(matrix + 0), acc0);
//				acc1 = _mm256_fmadd_ps(tmp1, _mm256_loadu_ps(matrix + 8), acc1);
//				acc2 = _mm256_fmadd_ps(tmp2, _mm256_loadu_ps(matrix + 16), acc2);
//				acc3 = _mm256_fmadd_ps(tmp3, _mm256_loadu_ps(matrix + 24), acc3);
//			}
//			acc0 = _mm256_add_ps(acc0, acc1);
//			acc2 = _mm256_add_ps(acc2, acc3);
//			acc0 = _mm256_add_ps(acc0, acc2);
//			acc0 = _mm256_max_ps(_mm256_setzero_ps(), acc0); // relu
//			__m256 tmp = _mm256_loadu_ps(output);
//			tmp = _mm256_mul_ps(tmp, acc0);
//			float x = horizontal_add(tmp) + bias[0];
//
			// approximate sigmoid:
			//   y = 0.5 + x / 4 + x^2 / 32	  for   x in range [-4, 0]
			//   y = 0.5 + x / 4 - x^2 / 32	  for   x in range [0, 4]
			// valid for x in range [-4, 4]
//			x = std::max(-4.0f, std::min(4.0f, x)); // clip to range [-4, 4]
//			const float a = 0.5f + 0.25f * x;
//			const float b = 0.03125f * x * x;
//			output[0] = (x > 0.0f) ? a - b : a + b;

//			_mm256_storeu_ps(output, acc0);

//			__m256 acc0 = _mm256_loadu_ps(bias);
//			__m256 acc1 = _mm256_setzero_ps();
//			__m256 acc2 = _mm256_setzero_ps();
//			__m256 acc3 = _mm256_setzero_ps();
//			for (int i = 0; i < InputLength; i += 4)
//			{
//				const __m128 in = _mm_loadu_ps(input + i);
//				const __m256 inp = _mm256_set_m128(in, in);
//				__m256 tmp0 = _mm256_shuffle_ps(inp, inp, 0x00);
//				__m256 tmp1 = _mm256_shuffle_ps(inp, inp, 0x55);
//				__m256 tmp2 = _mm256_shuffle_ps(inp, inp, 0xaa);
//				__m256 tmp3 = _mm256_shuffle_ps(inp, inp, 0xff);
//
//				acc0 = _mm256_fmadd_ps(tmp0, _mm256_loadu_ps(matrix), acc0);
//				acc1 = _mm256_fmadd_ps(tmp1, _mm256_loadu_ps(matrix), acc1);
//				acc2 = _mm256_fmadd_ps(tmp2, _mm256_loadu_ps(matrix), acc2);
//				acc3 = _mm256_fmadd_ps(tmp3, _mm256_loadu_ps(matrix), acc3);
//			}
//			acc0 = _mm256_add_ps(acc0, acc1);
//			acc2 = _mm256_add_ps(acc2, acc3);
//			acc0 = _mm256_add_ps(acc0, acc2);
//			acc0 = _mm256_max_ps(_mm256_setzero_ps(), acc0); // relu
//			_mm256_storeu_ps(output, acc0);

//			// acc[0] = [a0,a1,a2,a3,a4,a5,a6,a7]
//			// acc[1] = [b0,b1,b2,b3,b4,b5,b6,b7]
//			// acc[2] = [c0,c1,c2,c3,c4,c5,c6,c7]
//			// acc[3] = [d0,d1,d2,d3,d4,d5,d6,d7]
//			// acc[4] = [e0,e1,e2,e3,e4,e5,e6,e7]
//			// acc[5] = [f0,f1,f2,f3,f4,f5,f6,f7]
//			// acc[6] = [g0,g1,g2,g3,g4,g5,g6,g7]
//			// acc[7] = [h0,h1,h2,h3,h4,h5,h6,h7]
//			for (int j = 0; j < 4; j++)
//			{
//				__m256 tmp0 = _mm256_permute2f128_ps(acc[j], acc[j + 4], 0x20);
//				__m256 tmp1 = _mm256_permute2f128_ps(acc[j], acc[j + 4], 0x31);
//				acc[j] = _mm256_add_ps(tmp0, tmp1);
//			}
//			// acc[0] = [a04,a15,a26,a37,e04,e15,e26,e37] a and e
//			// acc[1] = [b04,b15,b26,b37,f04,f15,f26,f37] b and f
//			// acc[2] = [c04,c15,c26,c37,g04,g15,g26,g37] c and g
//			// acc[3] = [d04,d15,d26,d37,h04,h15,h26,h37] d and h
//			for (int j = 0; j < 4; j += 2)
//			{
//				__m256 tmp0 = _mm256_shuffle_ps(acc[j], acc[j + 1], 0x44); // 0101
//				__m256 tmp1 = _mm256_shuffle_ps(acc[j], acc[j + 1], 0xee); // 2323
//				acc[j] = _mm256_add_ps(tmp0, tmp1);
//				acc[j] = _mm256_shuffle_ps(acc[j], acc[j], 0xd8); //0213
//			}
//			// acc[0] = [a0426,b0426,a1537,b1537,e0426,f0426,e1537,f1537]
//			// acc[2] = [c0426,d0426,c1537,d1537,g0426,h0426,g1537,h1537]
//
//			__m256 tmp0 = _mm256_shuffle_ps(acc[0], acc[2], 0x44); // 0101
//			__m256 tmp1 = _mm256_shuffle_ps(acc[0], acc[2], 0xee); // 2323
//			// tmp0   = [a0426,b0426,c0426,d0426,e0426,f0426,g1537,h1537]
//			// tmp1   = [a1537,b1537,c1537,d1537,e1537,f1537,g0426,h0426]
//			_mm256_storeu_ps(output, _mm256_add_ps(tmp0, tmp1));

//			acc[0] = _mm256_add_ps(acc[0], acc[1]);
//			acc[2] = _mm256_add_ps(acc[2], acc[3]);
//			acc[0] = _mm256_add_ps(acc[0], acc[2]);
//			_mm256_storeu_ps(output, acc[0]);

//			const __m256i one = _mm256_set1_epi16(1);
//			__m256i acc[4];
//			for (int j = 0; j < 4; j++)
//				acc[j] = _mm256_setzero_si256();
//
//			__m256i mat[8];
//			const int8_t *ptr = reinterpret_cast<const int8_t*>(matrix);
//			for (int i = 0; i < InputLength; i += 2 * 4)
//			{
//				const __m128i tmp = _mm_loadu_si128((__m128i*) (input + i)); // load [i0,i1,i2,i3] - 16 int8
//				const __m256i in = _mm256_cvtepi16_epi32(tmp); // broadcast into [00,i0,00,i1,00,i2,00,i3]
//				{ // inner loop unrolled by 4
//					__m256i tmp0 = _mm256_shuffle_epi32(in, 0x00); // create [00,i0,00,i0,00,i0,00,i0]
//					__m256i tmp1 = _mm256_shuffle_epi32(in, 0x55); // create [00,i1,00,i1,00,i1,00,i1]
//					for (int j = 0; j < 8; j++, ptr += 16)
//					{
//						__m128i a = _mm_loadu_si128((__m128i*) ptr); // load 16 int8
//						mat[j] = _mm256_cvtepi16_epi32(a);
//					}
//
//					for (int j = 0; j < 4; j++)
//					{
//						mat[j] = _mm256_madd_epi16(tmp0, mat[j]); // first row
//						acc[j] = _mm256_add_epi32(acc[j], mat[j]);
//						mat[j + 4] = _mm256_madd_epi16(tmp1, mat[j + 4]); // second row
//						acc[j] = _mm256_add_epi32(acc[j], mat[j + 4]);
//					}
//
//					tmp0 = _mm256_shuffle_epi32(in, 0xaa); // create [i2,i2,i2,i2,i2,i2,i2,i2]
//					tmp1 = _mm256_shuffle_epi32(in, 0xff); // create [i3,i3,i3,i3,i3,i3,i3,i3]
//					for (int j = 0; j < 8; j++, ptr += 16)
//					{
//						__m128i a = _mm_loadu_si128((__m128i*) ptr); // load 16 int8
//						mat[j] = _mm256_cvtepi16_epi32(a);
//					}
//					for (int j = 0; j < 4; j++)
//					{
//						mat[j] = _mm256_madd_epi16(tmp0, mat[j]); // first row
//						mat[j + 4] = _mm256_madd_epi16(tmp1, mat[j + 4]); // second row
//						acc[j] = _mm256_add_epi32(acc[j], mat[j]);
//						acc[j] = _mm256_add_epi32(acc[j], mat[j + 4]);
//					}
//				}
//			}
//			for (int j = 0; j < 4; j++)
//			{
////				__m256 tmp = _mm256_cvtepi32_ps(acc[j]);
////				__m256 b = _mm256_loadu_ps(bias + j * 8);
////				tmp = _mm256_add_ps(tmp, b);
////				tmp = _mm256_max_ps(_mm256_setzero_ps(), tmp); // relu
////				_mm256_storeu_ps(output + j * 8, tmp);
//				_mm256_storeu_si256((__m256i*) (output + j * 8), acc[j]);
//			}

//			const __m256i in0 = _mm256_loadu_si256((__m256i*) (reinterpret_cast<const int8_t*>(input) + 0));
//			const __m256i in1 = _mm256_loadu_si256((__m256i*) (reinterpret_cast<const int8_t*>(input) + 32));
//
//			const int8_t *ptr = reinterpret_cast<const int8_t*>(matrix);
//			for (int i = 0; i < MatrixColumns; i += 8, ptr += 8 * 64)
//			{
//				const __m256i one = _mm256_set1_epi16(1);
//				__m256i acc[8];
//				for (int j = 0; j < 8; j++)
//				{
//					acc[j] = _mm256_maddubs_epi16(in0, _mm256_loadu_si256((__m256i*) (ptr + j * 64)));
//					acc[j] = _mm256_madd_epi16(one, acc[j]);
//				}
//				ptr += 1 * 32;
//				for (int j = 0; j < 8; j++)
//				{
//					__m256i tmp = _mm256_maddubs_epi16(in1, _mm256_loadu_si256((__m256i*) (ptr + j * 64)));
//					tmp = _mm256_madd_epi16(one, tmp);
//					acc[j] = _mm256_add_epi32(acc[j], tmp);
//				}
//
//				// acc[0] = [a0,a1,a2,a3,a4,a5,a6,a7]
//				// acc[1] = [b0,b1,b2,b3,b4,b5,b6,b7]
//				// acc[2] = [c0,c1,c2,c3,c4,c5,c6,c7]
//				// acc[3] = [d0,d1,d2,d3,d4,d5,d6,d7]
//				// acc[4] = [e0,e1,e2,e3,e4,e5,e6,e7]
//				// acc[5] = [f0,f1,f2,f3,f4,f5,f6,f7]
//				// acc[6] = [g0,g1,g2,g3,g4,g5,g6,g7]
//				// acc[7] = [h0,h1,h2,h3,h4,h5,h6,h7]
//				for (int j = 0; j < 4; j++)
//				{
//					__m256i tmp0 = _mm256_permute2x128_si256(acc[j], acc[j + 4], 0x20);
//					__m256i tmp1 = _mm256_permute2x128_si256(acc[j], acc[j + 4], 0x31);
//					acc[j] = _mm256_add_epi32(tmp0, tmp1);
//				}
//				// acc[0] = [a04,a15,a26,a37,e04,e15,e26,e37] a and e
//				// acc[1] = [b04,b15,b26,b37,f04,f15,f26,f37] b and f
//				// acc[2] = [c04,c15,c26,c37,g04,g15,g26,g37] c and g
//				// acc[3] = [d04,d15,d26,d37,h04,h15,h26,h37] d and h
//				for (int j = 0; j < 4; j += 2)
//				{
//					__m256i tmp0 = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(acc[j]), _mm256_castsi256_ps(acc[j + 1]), 0x44)); // 0101
//					__m256i tmp1 = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(acc[j]), _mm256_castsi256_ps(acc[j + 1]), 0xee)); // 2323
//					acc[j] = _mm256_add_epi32(tmp0, tmp1);
//					acc[j] = _mm256_shuffle_epi32(acc[j], 0xd8); //0213
//				}
//				// acc[0] = [a0426,b0426,a1537,b1537,e0426,f0426,e1537,f1537]
//				// acc[2] = [c0426,d0426,c1537,d1537,g0426,h0426,g1537,h1537]
//
//				__m256i tmp0 = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(acc[0]), _mm256_castsi256_ps(acc[2]), 0x44)); // 0101
//				__m256i tmp1 = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(acc[0]), _mm256_castsi256_ps(acc[2]), 0xee)); // 2323
//				// tmp0   = [a0426,b0426,c0426,d0426,e0426,f0426,g1537,h1537]
//				// tmp1   = [a1537,b1537,c1537,d1537,e1537,f1537,g0426,h0426]
//
//				__m256 tmp = _mm256_cvtepi32_ps(_mm256_add_epi32(tmp0, tmp1));
//				__m256 b = _mm256_loadu_ps(bias + i);
//				tmp = _mm256_add_ps(tmp, b);
//				tmp = _mm256_max_ps(_mm256_setzero_ps(), tmp); // relu
//
//				_mm256_storeu_ps(output + i, tmp);
//			}

//				__m256i acc[8];
//				for (int j = 0; j < 8; j++)
//					acc[j] = _mm256_loadu_si256((__m256i*) (ptr + j * 32));
//
//				for (int j = 0; j < 8; j += 2)
//				{
//					acc[j] = _mm256_maddubs_epi16(in0, acc[j]);
//					acc[j + 1] = _mm256_maddubs_epi16(in1, acc[j + 1]);
//				}
//				for (int j = 0; j < 8; j += 2)
//				{
//					acc[j] = _mm256_madd_epi16(one, acc[j]);
//					acc[j + 1] = _mm256_madd_epi16(one, acc[j + 1]);
//				}
//
//				for (int j = 0; j < 8; j += 2)
//					acc[j] = _mm256_add_epi32(acc[j], acc[j + 1]);
//
//				{
//					__m128i a = _mm_add_epi32(_mm256_extractf128_si256(acc[0], 1), _mm256_castsi256_si128(acc[0]));
//					__m128i b = _mm_add_epi32(_mm256_extractf128_si256(acc[2], 1), _mm256_castsi256_si128(acc[2]));
//					__m128i c = _mm_add_epi32(_mm256_extractf128_si256(acc[4], 1), _mm256_castsi256_si128(acc[4]));
//					__m128i d = _mm_add_epi32(_mm256_extractf128_si256(acc[6], 1), _mm256_castsi256_si128(acc[6]));
//
//					a = _mm_add_epi32(a, b);
//					c = _mm_add_epi32(c, d);
//					a = _mm_add_epi32(a, c);
//					_mm_storeu_si128((__m128i*) (reinterpret_cast<int32_t*>(output) + i), a);
//				}

//			}

//			const __m256i one = _mm256_set1_epi16(1);
//			__m256i acc0 = _mm256_loadu_si256((__m256i*) (bias + 0 * 8));
//			__m256i acc1 = _mm256_loadu_si256((__m256i*) (bias + 1 * 8));
//			__m256i acc2 = _mm256_loadu_si256((__m256i*) (bias + 2 * 8));
//			__m256i acc3 = _mm256_loadu_si256((__m256i*) (bias + 3 * 8));
//			const int32_t *ptr = reinterpret_cast<const int32_t*>(matrix);
//			for (int i = 0; i < InputLength; i += 4)
//			{
//				__m128i tmp = _mm_loadu_si128((__m128i*) (input + i));
//				const __m256i in = _mm256_set_m128i(_mm_(reinterpret_cast<const int32_t*>(input)[i]);
//				const __m256i mat0 = _mm256_loadu_si256((__m256i*) (ptr + 0 * 8));
//				const __m256i mat1 = _mm256_loadu_si256((__m256i*) (ptr + 1 * 8));
//				const __m256i mat2 = _mm256_loadu_si256((__m256i*) (ptr + 2 * 8));
//				const __m256i mat3 = _mm256_loadu_si256((__m256i*) (ptr + 3 * 8));
//				const __m256i tmp0 = _mm256_maddubs_epi16(in, mat0);
//				const __m256i tmp1 = _mm256_maddubs_epi16(in, mat1);
//				const __m256i tmp2 = _mm256_maddubs_epi16(in, mat2);
//				const __m256i tmp3 = _mm256_maddubs_epi16(in, mat3);
//				acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(one, tmp0));
//				acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(one, tmp1));
//				acc2 = _mm256_add_epi32(acc2, _mm256_madd_epi16(one, tmp2));
//				acc3 = _mm256_add_epi32(acc3, _mm256_madd_epi16(one, tmp3));
//			}
//			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output) + 0 * 8, acc0);
//			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output) + 1 * 8, acc1);
//			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output) + 2 * 8, acc2);
//			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output) + 3 * 8, acc3);

//			__m256i acc00, acc01;
//			__m256i acc10, acc11;
//			__m256i acc20, acc21;
//			__m256i acc30, acc31;
//			const int32_t *ptr = reinterpret_cast<const int32_t*>(matrix);
//			for (int c = 0; c < MatrixColumns; c += 16)
//			{
//				acc00 = _mm256_loadu_si256((__m256i*) (bias + c));
//				acc01 = _mm256_loadu_si256((__m256i*) (bias + c + 8));
//				acc10 = _mm256_setzero_si256();
//				acc11 = _mm256_setzero_si256();
//				acc20 = _mm256_setzero_si256();
//				acc21 = _mm256_setzero_si256();
//				acc30 = _mm256_setzero_si256();
//				acc31 = _mm256_setzero_si256();
//				for (int i = 0; i < InputLength; i += 8, ptr += 16)
//				{
//					const __m128i tmp = _mm_loadu_si128((__m128i*) (input + i)); // load [i0,i1,i2,i3]
//					const __m256i in = _mm256_set_m128i(tmp, tmp); // broadcast into [i0,i1,i2,i3,i0,i1,i2,i3]
//
//					const __m256i mat0 = _mm256_loadu_si256((__m256i*) ptr);
//					const __m256i mat1 = _mm256_loadu_si256((__m256i*) (ptr + 8));
//					{ // inner loop unrolled by 4
//						__m256i tmp0 = _mm256_shuffle_epi32(in, 0x00); // create [i0,i0,i0,i0,i0,i0,i0,i0]
//						__m256i tmp1 = _mm256_shuffle_epi32(in, 0x55); // create [i0,i0,i0,i0,i0,i0,i0,i0]
//						acc00 = _mm256_add_epi32(acc00, _mm256_madd_epi16(tmp0, mat0));
//						acc01 = _mm256_add_epi32(acc01, _mm256_madd_epi16(tmp0, mat1));
//						acc10 = _mm256_add_epi32(acc10, _mm256_madd_epi16(tmp1, mat0));
//						acc11 = _mm256_add_epi32(acc11, _mm256_madd_epi16(tmp1, mat1));
//						tmp0 = _mm256_shuffle_epi32(in, 0xaa); // create [i2,i2,i2,i2,i2,i2,i2,i2]
//						tmp1 = _mm256_shuffle_epi32(in, 0xff); // create [i3,i3,i3,i3,i3,i3,i3,i3]
//						acc20 = _mm256_add_epi32(acc20, _mm256_madd_epi16(tmp0, mat0));
//						acc21 = _mm256_add_epi32(acc21, _mm256_madd_epi16(tmp0, mat1));
//						acc30 = _mm256_add_epi32(acc30, _mm256_madd_epi16(tmp1, mat0));
//						acc31 = _mm256_add_epi32(acc31, _mm256_madd_epi16(tmp1, mat1));
//					}
//				}
//				acc00 = _mm256_add_epi32(acc00, acc10);
//				acc20 = _mm256_add_epi32(acc20, acc30);
//				acc00 = _mm256_add_epi32(acc00, acc20);
//				acc01 = _mm256_add_epi32(acc01, acc11);
//				acc21 = _mm256_add_epi32(acc21, acc31);
//				acc01 = _mm256_add_epi32(acc01, acc21);
//				_mm256_storeu_si256((__m256i*) (output + c), acc00);
//				_mm256_storeu_si256((__m256i*) (output + c + 8), acc01);
//			}

//			__m256 acc00, acc01, acc02, acc03;
//			__m256 acc10, acc11, acc12, acc13;
//			const float *ptr = matrix;
//			for (int c = 0; c < MatrixColumns; c += 32)
//			{
//				acc00 = _mm256_loadu_ps(bias + c);
//				acc01 = _mm256_loadu_ps(bias + c + 8);
//				acc02 = _mm256_loadu_ps(bias + c + 16);
//				acc03 = _mm256_loadu_ps(bias + c + 24);
//				acc10 = _mm256_setzero_ps();
//				acc11 = _mm256_setzero_ps();
//				acc12 = _mm256_setzero_ps();
//				acc13 = _mm256_setzero_ps();
//				for (int i = 0; i < InputLength; i += 4, ptr += 4 * 4 * 8)
//				{
//					const __m128 tmp = _mm_loadu_ps(input + i); // load [i0,i1,i2,i3]
//					const __m256 in = _mm256_set_m128(tmp, tmp); // broadcast into [i0,i1,i2,i3,i0,i1,i2,i3]
//
//					{ // inner loop unrolled by 4
//						__m256 tmp = _mm256_permute_ps(in, 0x00); // create [i0,i0,i0,i0,i0,i0,i0,i0]
//						acc00 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 0), tmp, acc00);
//						acc01 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 8), tmp, acc01);
//						acc02 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 16), tmp, acc02);
//						acc03 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 24), tmp, acc03);
//						ptr += 32;
//						tmp = _mm256_permute_ps(in, 0x55); // create [i1,i1,i1,i1,i1,i1,i1,i1]
//						acc10 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 0), tmp, acc10);
//						acc11 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 8), tmp, acc11);
//						acc12 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 16), tmp, acc12);
//						acc13 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 24), tmp, acc13);
//						ptr += 32;
//						tmp = _mm256_permute_ps(in, 0xaa); // create [i2,i2,i2,i2,i2,i2,i2,i2]
//						acc00 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 0), tmp, acc00);
//						acc01 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 8), tmp, acc01);
//						acc02 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 16), tmp, acc02);
//						acc03 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 24), tmp, acc03);
//						ptr += 32;
//						tmp = _mm256_permute_ps(in, 0xff); // create [i3,i3,i3,i3,i3,i3,i3,i3]
//						acc10 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 0), tmp, acc10);
//						acc11 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 8), tmp, acc11);
//						acc12 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 16), tmp, acc12);
//						acc13 = _mm256_fmadd_ps(_mm256_loadu_ps(ptr + 24), tmp, acc13);
//						ptr += 32;
//					}
//				}
//				acc00 = _mm256_add_ps(acc00, acc10);
//				acc01 = _mm256_add_ps(acc01, acc11);
//				acc02 = _mm256_add_ps(acc02, acc12);
//				acc03 = _mm256_add_ps(acc03, acc13);
//				_mm256_storeu_ps(output + c, acc00);
//				_mm256_storeu_ps(output + c + 8, acc01);
//				_mm256_storeu_ps(output + c + 16, acc02);
//				_mm256_storeu_ps(output + c + 24, acc03);
//			}

//			__m256 acc00, acc01;
//			__m256 acc10, acc11;
//			__m256 acc20, acc21;
//			__m256 acc30, acc31;
//			const float *ptr = matrix;
//			for (int c = 0; c < MatrixColumns; c += 16)
//			{
//				acc00 = _mm256_loadu_ps(bias + c);
//				acc01 = _mm256_loadu_ps(bias + c + 8);
//				acc10 = _mm256_setzero_ps();
//				acc11 = _mm256_setzero_ps();
//				acc20 = _mm256_setzero_ps();
//				acc21 = _mm256_setzero_ps();
//				acc30 = _mm256_setzero_ps();
//				acc31 = _mm256_setzero_ps();
//				for (int i = 0; i < InputLength; i += 4, ptr += 16)
//				{
//					const __m128 tmp = _mm_loadu_ps(input + i); // load [i0,i1,i2,i3]
//					const __m256 in = _mm256_set_m128(tmp, tmp); // broadcast into [i0,i1,i2,i3,i0,i1,i2,i3]
//
//					const __m256 mat0 = _mm256_loadu_ps(ptr);
//					const __m256 mat1 = _mm256_loadu_ps(ptr + 8);
//					{ // inner loop unrolled by 4
//						__m256 tmp0 = _mm256_permute_ps(in, 0x00); // create [i0,i0,i0,i0,i0,i0,i0,i0]
//						__m256 tmp1 = _mm256_permute_ps(in, 0x55); // create [i1,i1,i1,i1,i1,i1,i1,i1]
//						acc00 = _mm256_fmadd_ps(tmp0, mat0, acc00);
//						acc01 = _mm256_fmadd_ps(tmp0, mat1, acc01);
//						acc10 = _mm256_fmadd_ps(tmp1, mat0, acc10);
//						acc11 = _mm256_fmadd_ps(tmp1, mat1, acc11);
//						tmp0 = _mm256_permute_ps(in, 0xaa); // create [i2,i2,i2,i2,i2,i2,i2,i2]
//						tmp1 = _mm256_permute_ps(in, 0xff); // create [i3,i3,i3,i3,i3,i3,i3,i3]
//						acc20 = _mm256_fmadd_ps(tmp0, mat0, acc20);
//						acc21 = _mm256_fmadd_ps(tmp0, mat1, acc21);
//						acc30 = _mm256_fmadd_ps(tmp1, mat0, acc30);
//						acc31 = _mm256_fmadd_ps(tmp1, mat1, acc31);
//					}
//				}
//				acc00 = _mm256_add_ps(acc00, acc10);
//				acc20 = _mm256_add_ps(acc20, acc30);
//				acc00 = _mm256_add_ps(acc00, acc20);
//				acc01 = _mm256_add_ps(acc01, acc11);
//				acc21 = _mm256_add_ps(acc21, acc31);
//				acc01 = _mm256_add_ps(acc01, acc21);
//				_mm256_storeu_ps(output + c, acc00);
//				_mm256_storeu_ps(output + c + 8, acc01);
//			}

			for (int j = 0; j < MatrixColumns; j++)
				output[j] = bias[j];
			for (int i = 0; i < InputLength; i++)
			{
#ifdef __AVX__
				const __m256 in = _mm256_set1_ps(input[i]);
				for (int j = 0; j < MatrixColumns; j += 8)
				{
					__m256 out = _mm256_loadu_ps(output + j);
					const __m256 mat = _mm256_loadu_ps(matrix + i * MatrixColumns + j);
					out = _mm256_add_ps(out, _mm256_mul_ps(in, mat));
					_mm256_storeu_ps(output + j, out);
				}
#else
				const float tmp = input[i];
				for (int j = 0; j < MatrixColumns; j++)
					output[j] += tmp * matrix[i * MatrixColumns + j];
#endif
			}
		}
		void gemTv(const float *matrix, const float *gradientNext, float *gradientPrev, float *matrixUpdate, const float *input, int MatrixColumns,
				int InputLength)
		{
			assert(MatrixColumns % 8 == 0);
			assert(InputLength % 8 == 0);

			for (int i = 0; i < InputLength; i++)
			{
#ifdef __AVX__
				__m256 acc = _mm256_setzero_ps();
				const __m256 in = _mm256_set1_ps(input[i]);
				for (int j = 0; j < MatrixColumns; j += 8)
				{
					const __m256 grad = _mm256_loadu_ps(gradientNext + j);
					const __m256 mat = _mm256_loadu_ps(matrix + i * MatrixColumns + j);
					acc = _mm256_add_ps(acc, _mm256_mul_ps(grad, mat));

					__m256 mat_up = _mm256_loadu_ps(matrixUpdate + i * MatrixColumns + j);
					mat_up = _mm256_add_ps(mat_up, _mm256_mul_ps(in, grad));
					_mm256_storeu_ps(matrixUpdate + i * MatrixColumns + j, mat_up);
				}
				__m256 acc2 = _mm256_permute2f128_ps(acc, acc, 1);
				acc = _mm256_add_ps(acc, acc2);
				acc = _mm256_hadd_ps(acc, acc);
				acc = _mm256_hadd_ps(acc, acc);
				float tmp = _mm256_cvtss_f32(acc);
#else
				float tmp = 0.0f;
				const float in = input[i];
				for (int j = 0; j < MatrixColumns; j++)
				{
					tmp += gradientNext[j] * matrix[i * MatrixColumns + j];
					matrixUpdate[i * MatrixColumns + j] += in * gradientNext[j];
				}
#endif
				gradientPrev[i] = tmp;
			}
		}

	} /* namespace nnue */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_NNUE_GEMV_HPP_ */
