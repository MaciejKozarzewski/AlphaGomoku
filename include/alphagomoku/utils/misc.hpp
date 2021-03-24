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
#include <alphagomoku/mcts/Node.hpp>
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

	double getTime();

	bool isBoardFull(const matrix<Sign> &board);
	bool isBoardEmpty(const matrix<Sign> &board);

	matrix<Sign> boardFromString(std::string str);
	std::string boardToString(const matrix<Sign> &board, const Move &lastMove = Move());
	std::string policyToString(const matrix<Sign> &board, const matrix<float> &policy, const Move &lastMove = Move());

	int parseLine(char *line);
	std::string spaces(int number);
	std::string printStatistics(const char *name, uint64_t number, double time);

	template<typename T>
	void addVectors(std::vector<T> &dst, const std::vector<T> &src)
	{
		assert(dst.size() == src.size());
		for (size_t i = 0; i < dst.size(); i++)
			dst[i] += src[i];
	}

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy);
	void addNoise(const matrix<Sign> &board, matrix<float> &policy, float noiseWeight);

	void scaleArray(matrix<float> &array, float scale);

	float convertOutcome(GameOutcome outcome, Sign signToMove);
	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove);

	void averageStats(std::vector<float> &stats);

	const std::string currentDateTime();
	void logToFile(const char *file, const char *msg1, const char *msg2);

	Move pickMove(const matrix<float> &policy);
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
	std::vector<Move> prepareOpening(GameConfig config);
	Sign prepareOpening(GameRules rules, matrix<Sign> &board);

	void encodeInputTensor(float *dst, const matrix<Sign> &board, Sign signToMove);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MISC_HPP_ */
