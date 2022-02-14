/*
 * OpeningBook.hpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_OPENINGBOOK_HPP_
#define ALPHAGOMOKU_PLAYER_OPENINGBOOK_HPP_

#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/game/Move.hpp>
#include <inttypes.h>
#include <vector>

namespace ag
{

	class OpeningBook
	{
		struct OpeningEntry
		{
				// opening moves
				Move move1;
				Move move2;
				Move move3;
				// balancing moves
				Move move4;
				Move move5;
				// best 6th move
				Move move6;
				// evaluation
				Value evaluation3;
				Value evaluation5;
		};

		std::vector<OpeningEntry> openings;
	public:
		OpeningBook();
	};

} /* namespace ag */



#endif /* ALPHAGOMOKU_PLAYER_OPENINGBOOK_HPP_ */
