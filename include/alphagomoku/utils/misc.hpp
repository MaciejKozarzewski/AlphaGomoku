/*
 * misc.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_MISC_HPP_
#define ALPHAGOMOKU_UTILS_MISC_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>

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

	bool isBoardFull(const matrix<Sign> &board);
	bool isBoardEmpty(const matrix<Sign> &board);

	matrix<Sign> boardFromString(const std::string &str);
	std::string boardToString(const matrix<Sign> &board, const Move &lastMove = Move());
	std::string policyToString(const matrix<Sign> &board, const matrix<float> &policy, const Move &lastMove = Move());

	int parseLine(char *line);
	std::string spaces(int number);
	void print_statistics(const char *name, long number, double time);

	template<typename T>
	void addVectors(std::vector<T> &dst, const std::vector<T> &src)
	{
		assert(dst.size() == src.size());
		for (int i = 0; i < dst.size(); i++)
			dst[i] += src[i];
	}

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy);
	void addNoise(const matrix<Sign> &board, matrix<float> &policy, float noiseWeight);

	void scaleArray(matrix<float> &array, float scale);

	float convertOutcome(int outcome, int signToMove);
	void averageStats(std::vector<float> &stats);

	const std::string currentDateTime();
	void logToFile(const char *file, const char *msg1, const char *msg2);

	Move pickMove(const matrix<float> &policy);
	Move randomizeMove(const matrix<float> &policy);
	void normalize(matrix<float> &policy);
	float max(const matrix<float> &policy);

	template<typename T>
	void permute(T *begin, const T *end)
	{
		std::random_shuffle(begin, end);
	}

	std::vector<int> permutation(int length);

	void generateOpeningMap(const matrix<Sign> &board, matrix<float> &dist);

	void encodeInputTensor(float* dst, const matrix<Sign> &board, Sign signToMove);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MISC_HPP_ */
