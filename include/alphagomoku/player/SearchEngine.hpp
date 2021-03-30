/*
 * SearchEngine.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/ThreadPool.hpp>
#include <libml/utils/json.hpp>

namespace ag
{
	class GomocupPlayer;
}

namespace ag
{

	class SearchEngine
	{
		private:
			ThreadPool threads;
			Game game;

		public:
			SearchEngine(const Json &cfg, GameConfig gameConfig,GomocupPlayer &player);
			void setState(const Game &game);

			void stop();
			void makeMove();
			void ponder();
			void swap2();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_ */
