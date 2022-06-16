/*
 * misc.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_MISC_HPP_
#define ALPHAGOMOKU_UTILS_MISC_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>
#include <chrono>

namespace ag
{
	enum class GameRules
	;
	enum class GameOutcome
	;
}

namespace ag
{
	float randFloat();
	double randDouble();
	float randGaussian();
	int32_t randInt();
	int32_t randInt(int r);
	int32_t randInt(int r0, int r1);
	uint64_t randLong();
	bool randBool();

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
	/*
	 * \brief Calculates Gaussian cumulative distribution function.
	 */
	template<typename T>
	T gaussian_cdf(T x) noexcept
	{
		return static_cast<T>(0.5) * (static_cast<T>(1.0) + std::erf(x / std::sqrt(2.0)));
	}

	/*
	 * \brief Returns current time in seconds.
	 */
	inline double getTime()
	{
		return std::chrono::steady_clock::now().time_since_epoch().count() * 1.0e-9;
	}
	std::string currentDateTime();

	bool isBoardFull(const matrix<Sign> &board);
	bool isBoardEmpty(const matrix<Sign> &board);

	matrix<Sign> boardFromString(const std::string &str);
	std::vector<Move> extractMoves(const std::string &str);
	std::string boardToString(const matrix<Sign> &board, const Move &lastMove = Move());
	std::string provenValuesToString(const matrix<Sign> &board, const matrix<ProvenValue> &pv);
	std::string policyToString(const matrix<Sign> &board, const matrix<float> &policy);
	std::string actionValuesToString(const matrix<Sign> &board, const matrix<Value> &actionValues);

	std::string printStatistics(const char *name, uint64_t number, double time);

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

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy);
	void addNoise(const matrix<Sign> &board, matrix<float> &policy, float noiseWeight);
	matrix<float> getNoiseMatrix(const matrix<Sign> &board);

	void scaleArray(matrix<float> &array, float scale);
	std::vector<float> averageStats(std::vector<float> &stats);

	Move pickMove(const matrix<float> &policy);
	Move pickMove(const matrix<float> &policy, const matrix<ProvenValue> &provenValues);
	Move randomizeMove(const matrix<float> &policy, float temperature = 1.0f);
	void normalize(matrix<float> &policy);
	float max(const matrix<float> &policy);

	template<typename T>
	void permute(T *begin, const T *end)
	{
		std::random_shuffle(begin, end);
	}

	std::vector<int> permutation(int length);

	void generateOpeningMap(const matrix<Sign> &board, matrix<float> &dist);
	std::vector<Move> prepareOpening(GameConfig config, int minNumberOfMoves = 0);

	void encodeInputTensor(float *dst, const matrix<Sign> &board, Sign signToMove);

	std::string moveToString(const ag::Move &m);
	ag::Move moveFromString(const std::string &str, ag::Sign sign);
	bool startsWith(const std::string &line, const std::string &prefix);
	std::vector<std::string> split(const std::string &str, char delimiter);
	void toLowerCase(std::string &s);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MISC_HPP_ */
