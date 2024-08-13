/*
 * math_utils.hpp
 *
 *  Created on: Apr 24, 2023
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_MATH_UTILS_HPP_
#define ALPHAGOMOKU_UTILS_MATH_UTILS_HPP_

#include <alphagomoku/utils/matrix.hpp>

#include <vector>
#include <numeric>
#include <cmath>
#include <bitset>

namespace ag
{
	template<typename T>
	constexpr T clamp(T value, T lower, T upper) noexcept
	{
		assert(lower <= upper);
		return std::min(upper, std::max(lower, value));
	}

	template<typename T>
	constexpr bool isPowerOf2(T x) noexcept
	{
		return (x > 0) && !(x & (x - 1));
	}
	template<typename T>
	constexpr T square(T x) noexcept
	{
		return x * x;
	}
	template<typename T>
	T roundToPowerOf2(T x) noexcept
	{
		T result = 1;
		while (result <= x)
			result *= 2;
		return result / 2;
	}
	template<typename T>
	T roundToMultipleOf(T x, T mul) noexcept
	{
		const T remainder = x % mul;
		return (remainder == 0) ? x : (x + mul - remainder);
	}
	template<typename T>
	T popcount(T x) noexcept
	{
		return std::bitset<8 * sizeof(T)>(x).count();
	}
	template<typename T>
	T safe_log(T x) noexcept
	{
		return std::log(static_cast<T>(1.0e-16) + x);
	}

	template<typename T>
	T sigmoid(T x) noexcept
	{
		return static_cast<T>(1.0) / (static_cast<T>(1.0) + std::exp(-x));
	}

	template<typename T>
	void addVectors(std::vector<T> &dst, const std::vector<T> &src)
	{
		assert(dst.size() == src.size());
		for (size_t i = 0; i < dst.size(); i++)
			dst[i] += src[i];
	}
	template<typename T>
	void addMatrices(matrix<T> &dst, const matrix<T> &src)
	{
		assert(dst.size() == src.size());
		for (int i = 0; i < dst.size(); i++)
			dst.data()[i] += src.data()[i];
	}
	template<typename T>
	void scaleArray(matrix<T> &array, T scale)
	{
		for (int i = 0; i < array.size(); i++)
			array[i] *= scale;
	}
	template<typename T>
	void normalize(matrix<T> &policy)
	{
		float r = std::accumulate(policy.begin(), policy.end(), static_cast<T>(0));
		if (r == static_cast<T>(0))
			policy.fill(static_cast<T>(1) / policy.size());
		else
		{
			r = static_cast<T>(1) / r;
			for (int i = 0; i < policy.size(); i++)
				policy[i] *= r;
		}
	}
	template<typename T>
	void softmax(std::vector<T> &x)
	{
		T max_value = std::numeric_limits<T>::lowest();
		for (size_t i = 0; i < x.size(); i++)
			max_value = std::max(max_value, x[i]);

		T sum = static_cast<T>(0);
		for (size_t i = 0; i < x.size(); i++)
		{
			x[i] = std::exp(x[i] - max_value);
			sum += x[i];
		}

		assert(sum > static_cast<T>(0));
		sum = static_cast<T>(1) / sum;
		for (size_t i = 0; i < x.size(); i++)
			x[i] *= sum;
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MATH_UTILS_HPP_ */
