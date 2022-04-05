/*
 * ZobristHashing.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_
#define ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>

#include <vector>
#include <cinttypes>

namespace ag
{
	struct GameConfig;
} /* namespace ag */

namespace ag
{
	class ZobristHashing
	{
		private:
			std::vector<uint64_t> keys;
		public:
			ZobristHashing() = default;
			ZobristHashing(int boardHeight, int boardWidth);
			ZobristHashing(int boardHeight, int boardWidth, uint64_t seed);
			uint64_t getHash(const matrix<Sign> &board, Sign signToMove) const noexcept;
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_ */
