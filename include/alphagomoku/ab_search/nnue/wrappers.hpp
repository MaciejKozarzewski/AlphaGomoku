/*
 * wrappers.hpp
 *
 *  Created on: Oct 21, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_NNUE_WRAPPERS_HPP_
#define ALPHAGOMOKU_AB_SEARCH_NNUE_WRAPPERS_HPP_

//#include <alphagomoku/utils/AlignedStorage.hpp>

#include <cassert>
#include <cinttypes>

class Json;
class SerializedObject;

namespace ag
{
	namespace nnue
	{

		/*
		 * \brief Simple 1D wrapper for external memory block.
		 */
		template<typename T>
		class Wrapper1D
		{
				T *m_ptr = nullptr;
				int m_size = 0;
			public:
				Wrapper1D() noexcept = default;
				Wrapper1D(T *ptr, int size) :
						m_ptr(ptr),
						m_size(size)
				{
				}
				int size() const noexcept
				{
					return m_size;
				}
				int sizeInBytes() const noexcept
				{
					return sizeof(T) * size();
				}
				const T* data() const noexcept
				{
					return m_ptr;
				}
				T* data() noexcept
				{
					return m_ptr;
				}
				const T* begin() const noexcept
				{
					return data();
				}
				T* begin() noexcept
				{
					return data();
				}
				const T* end() const noexcept
				{
					return begin() + size();
				}
				T* end() noexcept
				{
					return begin() + size();
				}
				const T& operator[](int index) const noexcept
				{
					assert(0 <= index && index < size());
					return data()[index];
				}
				T& operator[](int index) noexcept
				{
					assert(0 <= index && index < size());
					return data()[index];
				}
		};

		/*
		 * \brief Simple 1D wrapper for external memory block.
		 */
		template<typename T>
		class Wrapper2D
		{
				T *m_ptr = nullptr;
				int m_rows = 0;
				int m_cols = 0;
			public:
				Wrapper2D() noexcept = default;
				Wrapper2D(T *ptr, int rows, int cols) :
						m_ptr(ptr),
						m_rows(rows),
						m_cols(cols)
				{
				}
				int rows() const noexcept
				{
					return m_rows;
				}
				int cols() const noexcept
				{
					return m_cols;
				}
				int size() const noexcept
				{
					return rows() * cols();
				}
				int sizeInBytes() const noexcept
				{
					return sizeof(T) * size();
				}
				const T* data() const noexcept
				{
					return m_ptr;
				}
				T* data() noexcept
				{
					return m_ptr;
				}
				const T* begin() const noexcept
				{
					return data();
				}
				T* begin() noexcept
				{
					return data();
				}
				const T* end() const noexcept
				{
					return begin() + size();
				}
				T* end() noexcept
				{
					return begin() + size();
				}
				const T& operator[](int index) const noexcept
				{
					assert(0 <= index && index < size());
					return data()[index];
				}
				T& operator[](int index) noexcept
				{
					assert(0 <= index && index < size());
					return data()[index];
				}
				const T* get_row(int row) const noexcept
				{
					assert(0 <= row && row < rows());
					return data() + row * cols();
				}
				T* get_row(int row) noexcept
				{
					assert(0 <= row && row < rows());
					return data() + row * cols();
				}
				const T& at(int row, int col) const noexcept
				{
					assert(0 <= row && row < rows());
					assert(0 <= col && col < cols());
					return data()[row * cols() + col];
				}
				T& at(int row, int col) noexcept
				{
					assert(0 <= row && row < rows());
					assert(0 <= col && col < cols());
					return data()[row * cols() + col];
				}
		};

		class QuantizedLayer
		{
				std::vector<int8_t> data;
			public:
				Wrapper2D<int8_t> weights;
				Wrapper1D<int32_t> bias;
				Wrapper1D<float> scale;

				QuantizedLayer() noexcept = default;
				QuantizedLayer(int inputs, int neurons);
				QuantizedLayer(const Json &json, const SerializedObject &so);
				Json save(SerializedObject &so) const;
				// output = (float)((int32_t)(input<uint8_t> * weights<int8_t>) + bias<int32_t>) * scale<float>
		};
		class RealSpaceLayer
		{
				std::vector<float> data;
			public:
				Wrapper2D<float> weights;
				Wrapper1D<float> bias;

				RealSpaceLayer() noexcept = default;
				RealSpaceLayer(int inputs, int neurons);
				RealSpaceLayer(const Json &json, const SerializedObject &so);
				Json save(SerializedObject &so) const;
		};

		/*
		 * approximate sigmoid
		 *   y = 0.5 + x / 4 - x^2 / 32	  for   x in range [-4, 0]
		 *   y = 0.5 + x / 4 + x^2 / 32	  for   x in range [0, 4]
		 * valid from x in range [-4, 4]
		 */
		static inline float approximate_sigmoid(float x) noexcept
		{
			x = std::max(-4.0f, std::min(4.0f, x)); // clip to range [-4, 4]
			const float a = 0.5f + 0.25f * x;
			const float b = 0.03125f * x * x;
			return (x >= 0) ? a - b : a + b;
		}
	}
}

#endif /* ALPHAGOMOKU_AB_SEARCH_NNUE_WRAPPERS_HPP_ */
