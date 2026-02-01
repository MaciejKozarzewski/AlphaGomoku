/*
 * matrix.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_MATRIX_HPP_
#define ALPHAGOMOKU_UTILS_MATRIX_HPP_

#include <vector>
#include <algorithm>
#include <cassert>

namespace ag
{
	struct MatrixShape
	{
			int rows = 0;
			int cols = 0;

			MatrixShape() noexcept = default;
			MatrixShape(int size) noexcept :
					MatrixShape(size, size)
			{
			}
			MatrixShape(int rows, int cols) noexcept
			{
				assert(rows >= 0);
				assert(cols >= 0);
				this->rows = rows;
				this->cols = cols;
			}

			bool isInside(int row, int col) const noexcept
			{
				return 0 <= row and row < rows and 0 <= col and col < cols;
			}
			bool isSquare() const noexcept
			{
				return rows == cols;
			}

			friend bool operator==(const MatrixShape &lhs, const MatrixShape &rhs) noexcept
			{
				return (lhs.rows == rhs.rows) and (lhs.cols == rhs.cols);
			}
			friend bool operator!=(const MatrixShape &lhs, const MatrixShape &rhs) noexcept
			{
				return not (lhs == rhs);
			}
	};

	template<typename T>
	class matrix
	{
		private:
			std::vector<T> m_data;
			MatrixShape m_shape;
		public:
			matrix() = default;
			matrix(int size) :
					matrix(size, size)
			{
			}
			matrix(int rows, int cols)
			{
				m_shape = MatrixShape(rows, cols);
				m_data.resize(rows * cols);
			}

			int rows() const noexcept
			{
				return m_shape.rows;
			}
			int cols() const noexcept
			{
				return m_shape.cols;
			}
			void clear()
			{
				m_data.assign(m_data.size(), T { });
			}
			void fill(const T &value)
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
			MatrixShape shape() const noexcept
			{
				return m_shape;
			}
			int size() const noexcept
			{
				return rows() * cols();
			}
			size_t sizeInBytes() const noexcept
			{
				return sizeof(T) * size();
			}
			void copyFrom(const matrix<T> &other)
			{
				assert(this->shape() == other.shape());
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
			const T& operator[](int idx) const noexcept
			{
				assert(idx >= 0 && idx < size());
				return m_data[idx];
			}
			T& operator[](int idx) noexcept
			{
				assert(idx >= 0 && idx < size());
				return m_data[idx];
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
				return shape().isSquare();
			}
			bool isInside(int row, int col) const noexcept
			{
				return shape().isInside(row, col);
			}

			friend bool operator==(const matrix<T> &lhs, const matrix<T> &rhs) noexcept
			{
				if (lhs.shape() != rhs.shape())
					return false;
				return std::equal(lhs.begin(), lhs.end(), rhs.begin());
			}
			friend bool operator!=(const matrix<T> &lhs, const matrix<T> &rhs) noexcept
			{
				return not (lhs == rhs);
			}
	};

	template<typename T, typename U>
	bool equal_shape(const matrix<T> &lhs, const matrix<U> &rhs) noexcept
	{
		return lhs.shape() == rhs.shape();
	}

	template<typename T>
	matrix<T> empty_like(const matrix<T> &x)
	{
		return matrix<T>(x.rows(), x.cols());
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MATRIX_HPP_ */
