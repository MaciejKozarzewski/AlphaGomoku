/*
 * ZobristHashing.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <cstddef>
#include <chrono>
#include <random>

namespace ag
{
	ZobristHashing::ZobristHashing(int boardHeight, int boardWidth) :
			ZobristHashing(boardHeight, boardWidth, std::chrono::system_clock::now().time_since_epoch().count())
	{
	}
	ZobristHashing::ZobristHashing(int boardHeight, int boardWidth, uint64_t seed) :
			keys(3 + 3 * boardHeight * boardWidth)
	{
		std::mt19937_64 generator(seed);
		for (size_t i = 0; i < keys.size(); i++)
			keys[i] = generator();
	}
	uint64_t ZobristHashing::getHash(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		assert(signToMove >= Sign::NONE && signToMove <= Sign::CIRCLE);
		uint64_t result = keys[static_cast<int>(signToMove)];

		assert(3u * (1u + board.size()) == keys.size());
		for (int i = 0, k = 3; i < board.size(); i++, k += 3)
		{
			assert(Sign::NONE <= board[i] && board[i] <= Sign::CIRCLE);
			result ^= keys[k + static_cast<int>(board[i])];
		}
		return result;
	}
} /* namespace ag */
