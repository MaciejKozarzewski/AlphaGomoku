/*
 * freestyle.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_RULES_FREESTYLE_HPP_
#define ALPHAGOMOKU_RULES_FREESTYLE_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	Sign getWhoWinsFreestyle(const matrix<Sign> &board);
	Sign getWhoWinsFreestyle(const matrix<Sign> &board, const Move &last_move);
}

#endif /* ALPHAGOMOKU_RULES_FREESTYLE_HPP_ */
