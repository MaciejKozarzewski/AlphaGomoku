/*
 * caro.hpp
 *
 *  Created on: Mar 4, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_RULES_CARO_HPP_
#define ALPHAGOMOKU_RULES_CARO_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	Sign getWhoWinsCaro(const matrix<Sign> &board);
	Sign getWhoWinsCaro(const matrix<Sign> &board, const Move &last_move);
}



#endif /* ALPHAGOMOKU_RULES_CARO_HPP_ */
