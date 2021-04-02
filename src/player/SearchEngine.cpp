/*
 * SearchEngine.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/GomocupPlayer.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <iostream>
#include <string>

namespace ag
{

	SearchEngine::SearchEngine(const Json &cfg, GameConfig gameConfig, GomocupPlayer &player) :
			threads(1),
			game_config(gameConfig),
			player(player)
	{

	}

	void SearchEngine::stop()
	{
	}
	void SearchEngine::makeMove(const matrix<Sign> &board, Sign signToMove)
	{
		std::cout << "BEFORE MOVE\n";
		std::cout << boardToString(board) << '\n';
		matrix<float> policy(board.rows(), board.cols());
		for (int i = 0; i < policy.rows(); i++)
			for (int j = 0; j < policy.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
					policy.at(i, j) = 1.0f;
		normalize(policy);
		Move m = randomizeMove(policy, 0.0f);
		m.sign = signToMove;
		player.makeMove(m);
		player.respondWith(moveToString(m));

		std::cout << "AFTER MOVE\n";
		std::cout << boardToString(board) << '\n';
	}
	void SearchEngine::ponder(const matrix<Sign> &board, Sign signToMove)
	{
	}
	void SearchEngine::swap2(const matrix<Sign> &board, Sign signToMove)
	{
		int placed_stones = board.size() - std::count(board.begin(), board.end(), Sign::NONE);
		std::cout << "placed stones = " << placed_stones << '\n';

		switch (placed_stones)
		{
			case 0:
				break; // place first three stones
			case 3:
				break; // find two balancing moves
			case 5:
				break; // calculate win rate and pick color
			default:
				break; // error
		}
	}

} /* namespace ag */

