/*
 * random.cpp
 *
 *  Created on: Apr 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/math_utils.hpp>

#include <random>
#include <chrono>
#include <cassert>

namespace
{
#ifdef NDEBUG
	thread_local std::mt19937 int32_generator(std::chrono::system_clock::now().time_since_epoch().count());
	thread_local std::mt19937_64 int64_generator(std::chrono::system_clock::now().time_since_epoch().count());
#else
	thread_local std::mt19937 int32_generator(0);
	thread_local std::mt19937_64 int64_generator(0);
#endif
}

namespace ag
{
	float randFloat()
	{
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		return dist(int32_generator);
	}
	double randDouble()
	{
		std::uniform_real_distribution<double> dist(0.0f, 1.0f);
		return dist(int64_generator);
	}
	float randGaussian()
	{
		return randGaussian(0.0f, 1.0f);
	}
	float randGaussian(float mean, float variance)
	{
		std::normal_distribution<float> dist(mean, variance);
		return dist(int32_generator);
	}
	int32_t randInt()
	{
		return int32_generator();
	}
	int32_t randInt(int r)
	{
		assert(r != 0);
		std::uniform_int_distribution<int32_t> dist(0, r - 1);
		return dist(int32_generator);
	}
	int32_t randInt(int r0, int r1)
	{
		assert(r0 != r1);
		std::uniform_int_distribution<int32_t> dist(r0, r1 - 1);
		return dist(int32_generator);
	}
	uint64_t randLong()
	{
		return int64_generator();
	}
	bool randBool()
	{
		return static_cast<bool>(int32_generator() & 1);
	}

	float randBeta(float alpha, float beta)
	{
		std::gamma_distribution<float> x_gamma(alpha);
		std::gamma_distribution<float> y_gamma(beta);
		const float X = x_gamma(int32_generator);
		const float Y = y_gamma(int32_generator);
		return X / (X + Y);
	}

	std::vector<int> permutation(int length)
	{
		std::vector<int> result(length);
		std::iota(result.begin(), result.end(), 0);
		std::random_shuffle(result.begin(), result.end());
		return result;
	}

	std::vector<float> createCustomNoise(size_t size)
	{
		std::vector<float> result(size, 0.0f);
		float sum = 0.0f;
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = pow(randFloat(), 4) * (1.0f - sum);
			sum += result[i];
		}
		std::random_shuffle(result.begin(), result.end());
		return result;
	}
	std::vector<float> createDirichletNoise(size_t size, float scale)
	{
		std::gamma_distribution<double> noise(scale);

		std::vector<float> result(size);
		float sum = 0.0f;
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = noise(int32_generator);
			sum += result[i];
		}
		sum = 1.0f / sum;
		for (size_t i = 0; i < result.size(); i++)
			result[i] *= sum;
		return result;
	}
	std::vector<float> createGumbelNoise(size_t size)
	{
		std::vector<float> result(size);
		for (size_t i = 0; i < result.size(); i++)
			result[i] = -log_eps(-log_eps(randFloat()));
		return result;
	}

} /* namespace ag */

