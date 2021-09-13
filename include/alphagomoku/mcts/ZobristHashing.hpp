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
#include <inttypes.h>

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
			ZobristHashing(GameConfig cfg);
			ZobristHashing(GameConfig cfg, uint64_t seed);
			uint64_t getHash(const matrix<Sign> &board, Sign signToMove) const noexcept;

			uint64_t getHash(const Board &board) const noexcept;
			void updateHash(uint64_t &hash, const Board &board, Move move) const noexcept;
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_ */
