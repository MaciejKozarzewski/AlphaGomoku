/*
 * bitmask.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_BITMASK_HPP_
#define ALPHAGOMOKU_UTILS_BITMASK_HPP_

#include <array>
#include <string>
#include <cassert>

namespace ag
{
	inline uint32_t reverse_bits(uint32_t x) noexcept
	{
		x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
		x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
		x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
		x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
		x = (x >> 16) | (x << 16);
		return x;
	}
	inline uint16_t reverse_bits(uint16_t x) noexcept
	{
		x = ((x >> 1) & 0x5555) | ((x & 0x5555) << 1);
		x = ((x >> 2) & 0x3333) | ((x & 0x3333) << 2);
		x = ((x >> 4) & 0x0F0F) | ((x & 0x0F0F) << 4);
		x = (x >> 8) | (x << 8);
		return x;
	}

	template<typename T>
	struct BitMask1D
	{
		private:
			T m_data = 0;
		public:
			class reference
			{
					friend class BitMask1D;
					T &m_entry;
					int m_idx;
					reference(T &entry, int idx) :
							m_entry(entry),
							m_idx(idx)
					{
					}
				public:
					operator bool() const noexcept
					{
						return static_cast<bool>((m_entry >> static_cast<T>(m_idx)) & static_cast<T>(1));
					}
					reference& operator=(bool b) noexcept
					{
						if (b)
							m_entry |= (static_cast<T>(1) << static_cast<T>(m_idx));
						else
							m_entry &= ~(static_cast<T>(1) << static_cast<T>(m_idx));
						return *this;
					}
			};

			BitMask1D() noexcept = default;
			BitMask1D(T data) noexcept :
					m_data(data)
			{
			}
			BitMask1D(const std::string &str)
			{
				for (size_t i = 0; i < str.size(); i++)
				{
					assert(str.at(i) == '1' || str.at(i) == '0');
					at(i) = (str.at(i) == '1');
				}
			}
			int size() const noexcept
			{
				return 8 * sizeof(T);
			}
			bool at(int idx) const noexcept
			{
				assert(0 <= idx && idx < size());
				return static_cast<bool>((m_data >> static_cast<T>(idx)) & static_cast<T>(1));
			}
			reference at(int idx) noexcept
			{
				assert(0 <= idx && idx < size());
				return reference(m_data, idx);
			}
			T operator[](int idx) const noexcept
			{
				assert(0 <= idx && idx < size());
				return static_cast<bool>((m_data >> static_cast<T>(idx)) & static_cast<T>(1));
			}
			reference operator[](int idx) noexcept
			{
				assert(0 <= idx && idx < size());
				return reference(m_data, idx);
			}
			T raw() const noexcept
			{
				return m_data;
			}
			void flip(int length = 8 * sizeof(T)) noexcept
			{
				m_data = reverse_bits(m_data) >> (8 * sizeof(T) - length);
			}
			friend bool operator==(const BitMask1D<T> &lhs, const BitMask1D<T> &rhs) noexcept
			{
				return lhs.raw() == rhs.raw();
			}
			friend bool operator!=(const BitMask1D<T> &lhs, const BitMask1D<T> &rhs) noexcept
			{
				return not (lhs == rhs);
			}
			friend BitMask1D<T>& operator&=(BitMask1D<T> &lhs, const BitMask1D<T> &rhs) noexcept
			{
				lhs.m_data &= rhs.m_data;
				return lhs;
			}
			friend BitMask1D<T>& operator|=(BitMask1D<T> &lhs, const BitMask1D<T> &rhs) noexcept
			{
				lhs.m_data |= rhs.m_data;
				return lhs;
			}
			BitMask1D<T> operator<<(int shift) const noexcept
			{
				return BitMask1D<T>(m_data << shift);
			}
			BitMask1D<T> operator>>(int shift) const noexcept
			{
				return BitMask1D<T>(m_data >> shift);
			}
	};

	template<typename T, int MaxRows = 32>
	class BitMask2D
	{
			std::array<T, MaxRows> m_data;
			int m_rows = 0;
			int m_columns = 0;
		public:
			class reference
			{
					friend class BitMask2D;
					T &m_entry;
					int m_idx;
					reference(T &entry, int idx) :
							m_entry(entry),
							m_idx(idx)
					{
					}
				public:
					operator bool() const noexcept
					{
						return static_cast<bool>((m_entry >> static_cast<T>(m_idx)) & static_cast<T>(1));
					}
					reference& operator=(bool b) noexcept
					{
						if (b)
							m_entry |= (static_cast<T>(1) << static_cast<T>(m_idx));
						else
							m_entry &= ~(static_cast<T>(1) << static_cast<T>(m_idx));
						return *this;
					}
			};

			BitMask2D() noexcept = default;
			BitMask2D(int rows, int cols) :
					m_rows(rows),
					m_columns(cols)
			{
				fill(false);
				assert(0 <= rows && rows < MaxRows);
				assert(0 <= cols && cols < static_cast<int>(8 * sizeof(T)));
			}
			void fill(bool b) noexcept
			{
				if (b)
					m_data.fill(static_cast<T>(-1));
				else
					m_data.fill(static_cast<T>(0));
			}
			int size() const noexcept
			{
				return rows() * cols();
			}
			int rows() const noexcept
			{
				return m_rows;
			}
			int cols() const noexcept
			{
				return m_columns;
			}
			const T* data() const noexcept
			{
				return m_data.data();
			}
			T* data() noexcept
			{
				return m_data.data();
			}
			bool at(int row, int col) const noexcept
			{
				assert(0 <= row && row < rows());
				assert(0 <= col && col < cols());
				return static_cast<bool>((m_data[row] >> static_cast<T>(col)) & static_cast<T>(1));
			}
			reference at(int row, int col) noexcept
			{
				assert(0 <= row && row < rows());
				assert(0 <= col && col < cols());
				return reference(m_data[row], col);
			}
			T operator[](int row) const noexcept
			{
				assert(0 <= row && row < rows());
				return m_data[row];
			}
			T& operator[](int row) noexcept
			{
				assert(0 <= row && row < rows());
				return m_data[row];
			}
			BitMask2D<T>& operator&=(const BitMask2D<T> &other) noexcept
			{
				assert(this->m_rows == other.m_rows);
				assert(this->m_columns == other.m_columns);
				assert(this->m_data.size() == other.m_data.size());
				for (size_t i = 0; i < m_data.size(); i++)
					m_data[i] &= other.m_data[i];
				return *this;
			}
			BitMask2D<T>& operator|=(const BitMask2D<T> &other) noexcept
			{
				assert(this->m_rows == other.m_rows);
				assert(this->m_columns == other.m_columns);
				assert(this->m_data.size() == other.m_data.size());
				for (size_t i = 0; i < m_data.size(); i++)
					m_data[i] |= other.m_data[i];
				return *this;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_BITMASK_HPP_ */
