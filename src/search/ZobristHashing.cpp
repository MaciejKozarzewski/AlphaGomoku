/*
 * ZobristHashing.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/ZobristHashing.hpp>

#include <cstddef>
#include <random>

namespace ag
{
	FullZobristHashing::FullZobristHashing(int boardHeight, int boardWidth) :
			m_keys(3 + 3 * boardHeight * boardWidth)
	{
		for (size_t i = 0; i < m_keys.size(); i++)
			m_keys[i].init();
	}
	HashKey64 FullZobristHashing::getHash(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		assert(signToMove >= Sign::NONE && signToMove <= Sign::CIRCLE);
		HashKey64 result = m_keys[static_cast<int>(signToMove)];

		assert(3u * (1u + board.size()) == m_keys.size());
		for (int i = 0, k = 3; i < board.size(); i++, k += 3)
		{
			assert(Sign::NONE <= board[i] && board[i] <= Sign::CIRCLE);
			result ^= m_keys[k + static_cast<int>(board[i])];
		}
		return result;
	}

	FastZobristHashing::FastZobristHashing(int boardHeight, int boardWidth) :
			m_keys(2 * boardHeight * boardWidth),
			m_rows(boardHeight),
			m_columns(boardWidth)
	{
		for (size_t i = 0; i < m_keys.size(); i++)
			m_keys[i].init();
	}
	int64_t FastZobristHashing::getMemory() const noexcept
	{
		return m_keys.size() * sizeof(HashKey128);
	}
	HashKey128 FastZobristHashing::getHash(const matrix<Sign> &board) const noexcept
	{
		assert(board.rows() == m_rows);
		assert(board.cols() == m_columns);
		assert(static_cast<int>(m_keys.size()) == 2 * board.size());

		HashKey128 result;
		for (int i = 0; i < board.size(); i++)
			switch (board[i])
			{
				default:
					break;
				case Sign::CROSS:
					result ^= m_keys[2 * i];
					break;
				case Sign::CIRCLE:
					result ^= m_keys[2 * i + 1];
					break;
			}
		return result;
	}

} /* namespace ag */
