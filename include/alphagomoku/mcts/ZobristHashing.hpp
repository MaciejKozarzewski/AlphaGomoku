/*
 * ZobristHashing.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_
#define ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <vector>
#include <inttypes.h>

namespace ag
{
	enum class Sign;
} /* namespace ag */

namespace ag
{
	class ZobristHashing
	{
		private:
			std::vector<uint64_t> keys;
		public:
			ZobristHashing(int boardSize, uint64_t seed = 0);
			uint64_t getHash(const matrix<Sign> &board, Sign signToMove) const noexcept;
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_ZOBRISTHASHING_HPP_ */
