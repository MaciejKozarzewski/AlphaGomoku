/*
 * matrix.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_MATRIX_HPP_
#define ALPHAGOMOKU_UTILS_MATRIX_HPP_

#include <vector>
#include <assert.h>
#include <cstring>
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
				m_data.clear();
			}
			void fill(T value)
			{
				std::fill(begin(), end(), value);
			}
			const T* data() const noexcept
			{
				return m_data.data();
			}
			T* data() noexcept
			{
				return m_data.data();
			}
			const T* data(int row_index) const noexcept
			{
				assert(row_index >= 0 && row_index < rows());
				return data() + row_index * cols();
			}
			T* data(int row_index) noexcept
			{
				assert(row_index >= 0 && row_index < rows());
				return data() + row_index * cols();
			}
			int size() const noexcept
			{
				return m_rows * m_cols;
			}
			int sizeInBytes() const noexcept
			{
				return sizeof(T) * size();
			}
			void copyFrom(const matrix<T> &other)
			{
				assert(this->rows() == other.rows());
				assert(this->cols() == other.cols());
				std::memcpy(this->data(), other.data(), sizeInBytes());
			}
			const T& at(int row_index, int col_index) const noexcept
			{
				assert(row_index >= 0 && row_index < rows());
				assert(col_index >= 0 && col_index < cols());
				return m_data[row_index * cols() + col_index];
			}
			T& at(int row_index, int col_index) noexcept
			{
				assert(row_index >= 0 && row_index < rows());
				assert(col_index >= 0 && col_index < cols());
				return m_data[row_index * cols() + col_index];
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
