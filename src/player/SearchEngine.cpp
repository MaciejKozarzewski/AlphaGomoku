/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/GomocupPlayer.hpp>

namespace ag
{

	SearchEngine::SearchEngine(const Json &cfg, GameConfig gameConfig, GomocupPlayer &player) :
			threads(1),
			game(gameConfig)
	{

	}
	void SearchEngine::setState(const Game &game)
	{
	}

	void SearchEngine::stop()
	{
	}
	void SearchEngine::makeMove()
	{
	}
	void SearchEngine::ponder()
	{
	}
	void SearchEngine::swap2()
	{
	}

} /* namespace ag */

