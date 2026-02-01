/*
 * random.hpp
 *
 *  Created on: Apr 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_RANDOM_HPP_
#define ALPHAGOMOKU_UTILS_RANDOM_HPP_

#include <alphagomoku/utils/math_utils.hpp>

#include <vector>
#include <algorithm>
#include <cinttypes>
#include <cmath>

namespace ag
{
	float randFloat();
	double randDouble();
	float randGaussian();
	float randGaussian(float mean, float stddev);
	int32_t randInt();
	int32_t randInt(int r);
	int32_t randInt(int r0, int r1);
	uint64_t randLong();
	bool randBool();

	float randBeta(float alpha, float beta);

	/*
	 * \brief Calculates Gaussian cumulative distribution function.
	 */
	template<typename T>
	T gaussian_cdf(T x, T mean = 0.0, T stddev = 1.0) noexcept
	{
		return static_cast<T>(0.5) * (static_cast<T>(1.0) + std::erf((x - mean) / (stddev * std::sqrt(static_cast<T>(2.0)))));
	}
	template<typename T>
	T inverse_erf(T y) noexcept
	{
		T x = static_cast<T>(0.5);
		for (int i = 0; i < 100; i++)
		{
			const T fx = std::erf(x) - y;
			const T dfx = 2.0 / std::sqrt(PI) * std::exp(-x * x);
			const T new_x = x - fx / dfx;

			if (new_x == x)
				break;

			x = new_x;
		}
		return x;
	}

	template<typename T>
	void permute(T *begin, const T *end)
	{
		std::random_shuffle(begin, end);
	}

	std::vector<int> permutation(int length);

	std::vector<float> createCustomNoise(size_t size);
	std::vector<float> createDirichletNoise(size_t size, float scale);
	std::vector<float> createGumbelNoise(size_t size);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_RANDOM_HPP_ */
