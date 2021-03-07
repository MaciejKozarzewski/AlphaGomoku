/*
 * ZobristHashing.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <bits/stdint-uintn.h>
#include <stddef.h>
#include <chrono>
#include <iostream>
#include <random>

namespace ag
{
	ZobristHashing::ZobristHashing(int board_size, uint64_t seed) :
			keys(3 + 3 * board_size)
	{
		if (seed == 0)
			seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937_64 generator(seed);
		for (size_t i = 0; i < keys.size(); i++)
			keys[i] = generator();
	}
	uint64_t ZobristHashing::getHash(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		uint64_t result = keys[static_cast<int>(signToMove)];
		for (int i = 0, k = 3; i < board.size(); i++, k += 3)
			result ^= keys[k + static_cast<int>(board.data()[i])];
		return result;
	}
}
