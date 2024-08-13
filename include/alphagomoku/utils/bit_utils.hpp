/*
 * bit_utils.hpp
 *
 *  Created on: Aug 12, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_BIT_UTILS_HPP_
#define ALPHAGOMOKU_UTILS_BIT_UTILS_HPP_

#include <cstring>

namespace ag
{

	template<typename T, typename U>
	T bitwise_cast(U x) noexcept
	{
		static_assert(sizeof(T) == sizeof(U), "Cannot cast types of different sizes");
		T result;
		std::memcpy(&result, &x, sizeof(T));
		return result;
	}

	template<uint64_t First, uint64_t Last, typename T>
	T extract_bits(T value) noexcept
	{
		static_assert(First <= Last, "");
		static_assert(0 <= First, "");
		static_assert(Last <= 8 * sizeof(T), "");
		constexpr T one = static_cast<T>(1);
		constexpr T mask = (one << (Last - First)) - one;
		return (value >> First) & mask;
	}

	template<uint64_t First, uint64_t Last, typename T, typename U>
	void insert_bits(T &value, U bits) noexcept
	{
		static_assert(First <= Last, "");
		static_assert(0 <= First, "");
		static_assert(Last <= 8 * sizeof(T), "");
		static_assert((Last - First) <= 8 * sizeof(U), "");
		constexpr T one = static_cast<T>(1);
		constexpr T mask = (one << (Last - First)) - one;
		value = (value & (~(mask << First))) | ((static_cast<T>(bits) & mask) << First);
	}
} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_BIT_UTILS_HPP_ */
