/*
 * random.hpp
 *
 *  Created on: Apr 24, 2023
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_RANDOM_HPP_
#define ALPHAGOMOKU_UTILS_RANDOM_HPP_

#include <vector>
#include <algorithm>
#include <cinttypes>
#include <cmath>

namespace ag
{
	float randFloat();
	double randDouble();
	float randGaussian();
	float randGaussian(float mean, float variance);
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
	T gaussian_cdf(T x) noexcept
	{
		return static_cast<T>(0.5) * (static_cast<T>(1.0) + std::erf(x / std::sqrt(static_cast<T>(2.0))));
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
