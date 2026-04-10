/*
 * TwoMatch.hpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_TWOMATCH_HPP_
#define ALPHAGOMOKU_EVALUATION_TWOMATCH_HPP_

#include <alphagomoku/game/Game.hpp>

namespace ag
{

	class TwoMatch
	{
			Game first_game, second_game;
		public:
			TwoMatch(GameConfig gameConfig);
			void reset();
			GameConfig getConfig() const noexcept;
			const Game& getGame(int idx) const;
			Game& getGame(int idx);
			bool isOver() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_TWOMATCH_HPP_ */
