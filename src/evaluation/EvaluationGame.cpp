/*
 * EvaluationGame.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/evaluation/EvaluationGame.hpp>
#include <alphagomoku/evaluation/EvaluationManager.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <minml/utils/json.hpp>

namespace ag
{
	EvaluationGame::EvaluationGame(GameConfig gameConfig, EvaluatorThread &manager, bool useOpening) :
			manager_thread(manager),
			match(gameConfig),
			opening_generator(gameConfig, 8),
			use_opening(useOpening)
	{
	}
	void EvaluationGame::clear()
	{
		first_player.reset();
		second_player.reset();
		state = GAME_NOT_STARTED;
		match.reset();
		currently_played_game = 0;
	}
	void EvaluationGame::setFirstPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name)
	{
		first_player = std::make_unique<Player>(match.getConfig(), options, evaluator, name);
	}
	void EvaluationGame::setSecondPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name)
	{
		second_player = std::make_unique<Player>(match.getConfig(), options, evaluator, name);
	}
	bool EvaluationGame::prepareOpening()
	{
		if (first_player == nullptr)
			throw std::logic_error("EvaluationGame::prepareOpening() first_player is null");

		if (use_opening and currently_played_game == 0)
		{
			if (opening_generator.isEmpty())
				opening_generator.generate(8, first_player->getNNEvaluator(), first_player->getSolver());

			if (opening_generator.isEmpty())
				return false;
			else
				opening = opening_generator.pop();
		}

		if (currently_played_game == 0)
		{
			first_player->setSign(Sign::CROSS);
			second_player->setSign(Sign::CIRCLE);
			get_game().setPlayerNames(first_player->getName(), second_player->getName());
			get_game().loadOpening(opening);
		}
		else
		{
			assert(currently_played_game == 1);
			first_player->setSign(Sign::CIRCLE);
			second_player->setSign(Sign::CROSS);
			get_game().setPlayerNames(second_player->getName(), first_player->getName());
			get_game().loadOpening(opening);
		}
		return true;
	}
	void EvaluationGame::generate(int p)
	{
		if (first_player == nullptr)
			throw std::logic_error("EvaluationGame::generate() first_player is null");
		if (second_player == nullptr)
			throw std::logic_error("EvaluationGame::generate() second_player is null");

		if (state == GAME_NOT_STARTED)
		{
			first_player->getSolver().clear();
			second_player->getSolver().clear();

			first_player->newGame();
			second_player->newGame();
			state = PREPARE_OPENING;
		}

		if (state == PREPARE_OPENING)
		{
			const bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
			{
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				get_player().setBoard(get_game().getBoard(), get_game().getSignToMove());
			}
		}

		switch (p)
		{
			case 1: // run generation only for the first player
				if (get_game().getSignToMove() == first_player->getSign())
					break;
				else
					return;
			case 2: // run generation only for the second player
				if (get_game().getSignToMove() == second_player->getSign())
					break;
				else
					return;
			default:
			case 3: // run generation for both players
				break;
		}

		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE)
		{
			get_player().selectSolveEvaluate();
			state = GAMEPLAY_EXPAND_AND_BACKUP;
			return;
		}

		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			get_player().expandBackup();
			if (get_player().isSearchOver())
			{
				get_game().makeMove(get_player().getMove());
				if (get_game().isOver())
				{
					currently_played_game = 1 - currently_played_game;
					state = GAME_NOT_STARTED;
					if (match.isOver())
					{
						manager_thread.addToBuffer(match);
						match.reset();
					}
					return;
				}
				else
					get_player().setBoard(get_game().getBoard(), get_game().getSignToMove());

			}
			state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
		}
	}
	/*
	 * private
	 */
	Player& EvaluationGame::get_player() noexcept
	{
		assert(first_player != nullptr && first_player->getSign() != Sign::NONE);
		assert(second_player != nullptr && second_player->getSign() != Sign::NONE);
		assert(first_player->getSign() == invertSign(second_player->getSign()));

		if (get_game().getSignToMove() == first_player->getSign())
			return *first_player;
		else
			return *second_player;
	}
	Game& EvaluationGame::get_game() noexcept
	{
		return match.getGame(currently_played_game);
	}
} /* namespace ag */

