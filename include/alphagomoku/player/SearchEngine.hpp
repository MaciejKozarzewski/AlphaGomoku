/*
 * SearchEngine.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_
#define ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_

#include <alphagomoku/utils/ThreadPool.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <libml/utils/json.hpp>

namespace ag
{
	class Game;
	class GomocupPlayer;
}

namespace ag
{

	class SearchEngine
	{
		private:
			ThreadPool threads;
			GameConfig game_config;
			GomocupPlayer &player;

//			Tree tree;
//			Cache cache;

		public:
			SearchEngine(const Json &cfg, GomocupPlayer &player);

			void stop();
			void makeMove(const matrix<Sign> &board, Sign signToMove);
			void ponder(const matrix<Sign> &board, Sign signToMove);
			void swap2(const matrix<Sign> &board, Sign signToMove);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_SEARCHENGINE_HPP_ */
