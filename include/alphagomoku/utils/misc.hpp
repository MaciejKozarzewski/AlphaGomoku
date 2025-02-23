/*
 * misc.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_MISC_HPP_
#define ALPHAGOMOKU_UTILS_MISC_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cassert>
#include <cstdlib>
#include <string>
#include <chrono>

namespace ag
{
	enum class GameRules;
	enum class GameOutcome;
	struct GameConfig;
}

namespace ag
{
	struct float3
	{
			float x, y, z;
	};

	template<typename T>
	bool is_aligned(const void *ptr) noexcept
	{
		return (reinterpret_cast<std::uintptr_t>(ptr) % sizeof(T)) == 0;
	}

	/*
	 * \brief Returns current time in seconds.
	 */
	inline double getTime()
	{
		return std::chrono::steady_clock::now().time_since_epoch().count() * 1.0e-9;
	}
	std::string currentDateTime();

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy);

	std::vector<float> averageStats(std::vector<float> &stats);

	Move pickMove(const matrix<float> &policy);
	Move randomizeMove(const matrix<float> &policy);
	float max(const matrix<float> &policy);

	void generateOpeningMap(const matrix<Sign> &board, matrix<float> &dist);
	std::vector<Move> prepareOpening(const GameConfig &config, int minNumberOfMoves = 0);
	/*
	 * Function used to calculate reduced number of simulations if draw probability exceeds certain threshold.
	 */
	int get_simulations_for_move(float drawRate, int maxSimulations, int minSimulations) noexcept;

	std::string zfill(int value, int length);
	std::string sfill(int value, int length, bool isSigned);
	bool startsWith(const std::string &line, const std::string &prefix);
	std::vector<std::string> split(const std::string &str, char delimiter);
	void toLowerCase(std::string &s);

	std::string format_percents(double x);
	std::string trim(const std::string &str);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MISC_HPP_ */
