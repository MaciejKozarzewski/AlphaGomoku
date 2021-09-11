/*
 * ZobristHashing.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <inttypes.h>
#include <stddef.h>
#include <chrono>
#include <random>

namespace ag
{
	ZobristHashing::ZobristHashing(GameConfig cfg) :
			keys(3 + 3 * cfg.rows * cfg.cols)
	{
		std::mt19937_64 generator(std::chrono::system_clock::now().time_since_epoch().count());
		for (size_t i = 0; i < keys.size(); i++)
			keys[i] = generator();
	}
	ZobristHashing::ZobristHashing(GameConfig cfg, uint64_t seed) :
			keys(3 + 3 * cfg.rows * cfg.cols)
	{
		std::mt19937_64 generator(seed);
		for (size_t i = 0; i < keys.size(); i++)
			keys[i] = generator();
	}
	uint64_t ZobristHashing::getHash(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		assert(3u * (1 + board.size()) == keys.size());
		uint64_t result = keys[static_cast<int>(signToMove)];
		for (int i = 0, k = 3; i < board.size(); i++, k += 3)
			result ^= keys[k + static_cast<int>(board.data()[i])];
		return result;
	}
	uint64_t ZobristHashing::getHash(const Board &board) const noexcept
	{
		assert(3u * (1 + board.size()) == keys.size());
		uint64_t result = keys[static_cast<int>(board.signToMove())];
		for (int i = 0, k = 3; i < board.size(); i++, k += 3)
			result ^= keys[k + static_cast<int>(board.data()[i])];
		return result;
	}
} /* namespace ag */
