/*
 * common.hpp
 *
 *  Created on: Oct 6, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_COMMON_HPP_
#define ALPHAGOMOKU_PATTERNS_COMMON_HPP_

#include <cinttypes>
#include <cassert>

namespace ag
{
	typedef int Direction;
	const Direction HORIZONTAL = 0;
	const Direction VERTICAL = 1;
	const Direction DIAGONAL = 2;
	const Direction ANTIDIAGONAL = 3;

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
	struct DirectionGroup
	{
			T horizontal = T { };
			T vertical = T { };
			T diagonal = T { };
			T antidiagonal = T { };

			T operator[](Direction direction) const noexcept
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
				return (horizontal == value) or (vertical == value) or (diagonal == value) or (antidiagonal == value);
			}
			Direction findDirectionOf(T value) const noexcept
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
			DirectionGroup<T> for_cross;
			DirectionGroup<T> for_circle;
	};

	template<typename T, int MaxSize>
	class ShortVector
	{
			std::array<T, MaxSize> m_data;
			int m_size = 0;
		public:
			ShortVector() noexcept = default;
			ShortVector(std::initializer_list<T> list) :
				m_size(list.size())
			{
				for (int i = 0; i < m_size; i++)
					m_data[i] = list.begin()[i];
			}
			void add(const T &value) noexcept
			{
				assert(m_size < MaxSize);
				m_data[m_size++] = value;
			}
			int size() const noexcept
			{
				return m_size;
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
				return m_data.data();
			}
			T* end() noexcept
			{
				return begin() + size();
			}
			const T* begin() const noexcept
			{
				return m_data.data();
			}
			const T* end() const noexcept
			{
				return begin() + size();
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_COMMON_HPP_ */
