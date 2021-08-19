/*
 * matrix.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_MATRIX_HPP_
#define ALPHAGOMOKU_UTILS_MATRIX_HPP_

#include <vector>
#include <assert.h>
#include <memory>

namespace ag
{
	template<typename T>
	class matrix
	{
		private:
			std::vector<T> m_data;
			int m_rows = 0;
			int m_cols = 0;
		public:

			matrix() = default;
			matrix(int rows, int cols) :
					m_data(rows * cols),
					m_rows(rows),
					m_cols(cols)
			{
				assert(rows >= 0);
				assert(cols >= 0);
			}

			int rows() const noexcept
			{
				return m_rows;
			}
			int cols() const noexcept
			{
				return m_cols;
			}
			void clear()
			{
				m_data.assign(m_data.size(), T { });
			}
			void fill(T value)
			{
				m_data.assign(m_data.size(), value);
			}
			const T* data() const noexcept
			{
				return m_data.data();
			}
			T* data() noexcept
			{
				return m_data.data();
			}
			const T* data(int row) const noexcept
			{
				assert(row >= 0 && row < rows());
				return data() + row * cols();
			}
			T* data(int row) noexcept
			{
				assert(row >= 0 && row < rows());
				return data() + row * cols();
			}
			int size() const noexcept
			{
				return rows() * cols();
			}
			int sizeInBytes() const noexcept
			{
				return sizeof(T) * size();
			}
			void copyFrom(const matrix<T> &other)
			{
				assert(this->rows() == other.rows());
				assert(this->cols() == other.cols());
				this->m_data = other.m_data;
			}
			const T& at(int row, int col) const noexcept
			{
				assert(row >= 0 && row < rows());
				assert(col >= 0 && col < cols());
				return m_data[row * cols() + col];
			}
			T& at(int row, int col) noexcept
			{
				assert(row >= 0 && row < rows());
				assert(col >= 0 && col < cols());
				return m_data[row * cols() + col];
			}

			T* begin() noexcept
			{
				return data();
			}
			const T* begin() const noexcept
			{
				return data();
			}
			T* end() noexcept
			{
				return data() + size();
			}
			const T* end() const noexcept
			{
				return data() + size();
			}
			bool isSquare() const noexcept
			{
				return rows() == cols();
			}

			friend bool operator==(const matrix<T> &lhs, const matrix<T> &rhs)
			{
				if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols())
					return false;
				return std::equal(lhs.begin(), lhs.end(), rhs.begin());
			}
			friend bool operator!=(const matrix<T> &lhs, const matrix<T> &rhs)
			{
				return !(lhs == rhs);
			}

	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MATRIX_HPP_ */
