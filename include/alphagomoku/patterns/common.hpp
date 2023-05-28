/*
 * common.hpp
 *
 *  Created on: Oct 6, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_COMMON_HPP_
#define ALPHAGOMOKU_PATTERNS_COMMON_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/AlignedAllocator.hpp>

#include <vector>
#include <cinttypes>
#include <cassert>

namespace ag
{
	template<typename T, int N>
	constexpr T ones() noexcept
	{
		return (static_cast<T>(1) << N) - static_cast<T>(1);
	}

	typedef int Direction;
	constexpr Direction HORIZONTAL = 0;
	constexpr Direction VERTICAL = 1;
	constexpr Direction DIAGONAL = 2;
	constexpr Direction ANTIDIAGONAL = 3;

	template<typename T>
	Location shiftInDirection(Direction dir, int distance, T origin) noexcept
	{
		switch (dir)
		{
			case HORIZONTAL:
				return Location(origin.row, origin.col + distance);
			case VERTICAL:
				return Location(origin.row + distance, origin.col);
			case DIAGONAL:
				return Location(origin.row + distance, origin.col + distance);
			case ANTIDIAGONAL:
				return Location(origin.row + distance, origin.col - distance);
			default:
				return Location(origin.row, origin.col);
		}
	}

	inline constexpr int get_row_step(Direction dir) noexcept
	{
		assert(HORIZONTAL <= dir && dir <= ANTIDIAGONAL);
		return (dir == HORIZONTAL) ? 0 : 1;
	}
	inline constexpr int get_col_step(Direction dir) noexcept
	{
		assert(HORIZONTAL <= dir && dir <= ANTIDIAGONAL);
		switch (dir)
		{
			case HORIZONTAL:
				return 1;
			case VERTICAL:
				return 0;
			case DIAGONAL:
				return 1;
			case ANTIDIAGONAL:
				return -1;
			default:
				return 0;
		}
	}

	template<typename T>
	struct alignas(4 * sizeof(T)) DirectionGroup
	{
			T horizontal = T { };
			T vertical = T { };
			T diagonal = T { };
			T antidiagonal = T { };

			const T& operator[](Direction direction) const noexcept
			{
				assert(HORIZONTAL <= direction && direction <= ANTIDIAGONAL);
				switch (direction)
				{
					default:
					case HORIZONTAL:
						return horizontal;
					case VERTICAL:
						return vertical;
					case DIAGONAL:
						return diagonal;
					case ANTIDIAGONAL:
						return antidiagonal;
				}
			}
			T& operator[](Direction direction) noexcept
			{
				assert(HORIZONTAL <= direction && direction <= ANTIDIAGONAL);
				switch (direction)
				{
					default:
					case HORIZONTAL:
						return horizontal;
					case VERTICAL:
						return vertical;
					case DIAGONAL:
						return diagonal;
					case ANTIDIAGONAL:
						return antidiagonal;
				}
			}
			int count(T value) const noexcept
			{
				return static_cast<int>(horizontal == value) + static_cast<int>(vertical == value) + static_cast<int>(diagonal == value)
						+ static_cast<int>(antidiagonal == value);
			}
			bool contains(T value) const noexcept
			{
				return count(value) > 0;
			}
			T max() const noexcept
			{
				return std::max(std::max(horizontal, vertical), std::max(diagonal, antidiagonal));
			}
			Direction findDirectionOf(T value) const noexcept // TODO find out if this can be optimized
			{
				if (horizontal == value)
					return HORIZONTAL;
				if (vertical == value)
					return VERTICAL;
				if (diagonal == value)
					return DIAGONAL;
				if (antidiagonal == value)
					return ANTIDIAGONAL;
				assert(false); // the searched pattern must exist in the group
				return -1;
			}
	};

	template<typename T>
	struct TwoPlayerGroup
	{
			T for_cross;
			T for_circle;
	};

	template<typename T, int N>
	class ShortVector
	{
			T m_data[N];
			int m_size = 0;
		public:
			ShortVector() noexcept = default;
			ShortVector(std::initializer_list<T> list) :
					m_size(list.size())
			{
				for (int i = 0; i < m_size; i++)
					m_data[i] = list.begin()[i];
			}
			bool contains(const T &value) const noexcept
			{
				for (int i = 0; i < size(); i++)
					if (m_data[i] == value)
						return true;
				return false;
			}
			int find(const T &value) const noexcept
			{
				for (int i = 0; i < size(); i++)
					if (m_data[i] == value)
						return i;
				return -1;
			}
			void clear() noexcept
			{
				m_size = 0;
			}
			void add(const T &value) noexcept
			{
				assert(m_size < capacity());
				m_data[m_size++] = value;
			}
			template<int M>
			void add(const ShortVector<T, M> &other) noexcept
			{
				assert(size() + M <= N);
				for (int i = 0; i < other.size(); i++)
					add(other[i]);
			}
			void remove(const T &value) noexcept
			{
				for (int i = 0; i < size(); i++)
					if (m_data[i] == value)
					{
						m_data[i] = m_data[--m_size]; // move the element to remove to the end and decrement size
						return;
					}
			}
			void remove(int index) noexcept
			{
				assert(0 <= index && index < m_size);
				m_data[index] = m_data[--m_size]; // move the element to remove to the end and decrement size
			}
			int size() const noexcept
			{
				return m_size;
			}
			int capacity() const noexcept
			{
				return N;
			}
			const T& operator[](int index) const noexcept
			{
				assert(0 <= index && index < size());
				return m_data[index];
			}
			T& operator[](int index) noexcept
			{
				assert(0 <= index && index < size());
				return m_data[index];
			}
			T* begin() noexcept
			{
				return m_data;
			}
			T* end() noexcept
			{
				return begin() + size();
			}
			const T* begin() const noexcept
			{
				return m_data;
			}
			const T* end() const noexcept
			{
				return begin() + size();
			}
	};

	class LocationList: public std::vector<Location, AlignedAllocator<Location, 64>>
	{
		public:
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_COMMON_HPP_ */
