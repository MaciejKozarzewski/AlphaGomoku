/*
 * TwoMatch.cpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/evaluation/TwoMatch.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <minml/utils/json.hpp>

namespace ag
{
	TwoMatch::TwoMatch(GameConfig gameConfig) :
			first_game(gameConfig),
			second_game(gameConfig)
	{
	}
	void TwoMatch::reset()
	{
		first_game.beginGame();
		second_game.beginGame();
	}
	GameConfig TwoMatch::getConfig() const noexcept
	{
		return first_game.getConfig();
	}
	const Game& TwoMatch::getGame(int idx) const
	{
		assert(idx == 0 || idx == 1);
		return (idx == 0) ? first_game : second_game;
	}
	Game& TwoMatch::getGame(int idx)
	{
		assert(idx == 0 || idx == 1);
		return (idx == 0) ? first_game : second_game;
	}
	bool TwoMatch::isOver() const
	{
		return first_game.isOver() and second_game.isOver();
	}

} /* namespace ag */




