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
			ZobristHashing(cfg, std::chrono::system_clock::now().time_since_epoch().count())
	{
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
	void ZobristHashing::updateHash(uint64_t &hash, const Board &board, Move move) const noexcept
	{
		assert(hash == getHash(board));
		hash ^= keys[static_cast<int>(move.sign)]; // XOR-ing out previous sign to move
		hash ^= keys[static_cast<int>(invertSign(move.sign))]; // XOR-ing in new sign to move

		if (board.at(move.row, move.col) == Sign::NONE)
		{
			assert(move.sign != Sign::NONE);
			assert(move.sign == board.signToMove());
			hash ^= keys[3 * (1 + move.row * board.cols() + move.col) + static_cast<int>(Sign::NONE)];
			hash ^= keys[3 * (1 + move.row * board.cols() + move.col) + static_cast<int>(move.sign)];
		}
		else
		{
			assert(board.at(move.row, move.col) == move.sign);
			assert(invertSign(move.sign) == board.signToMove());
			hash ^= keys[3 * (1 + move.row * board.cols() + move.col) + static_cast<int>(board.at(move.row, move.col))];
			hash ^= keys[3 * (1 + move.row * board.cols() + move.col) + static_cast<int>(Sign::NONE)];
		}
	}

} /* namespace ag */
