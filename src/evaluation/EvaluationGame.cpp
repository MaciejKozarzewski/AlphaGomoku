/*
 * EvaluationGame.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#include <alphagomoku/evaluation/EvaluationGame.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <libml/utils/json.hpp>

namespace ag
{

	Player::Player(GameConfig gameOptions, const Json &options, EvaluationQueue &queue, const std::string &name) :
			queue(queue),
			game_config(gameOptions),
			tree(TreeConfig(options["tree_options"])),
			cache(gameOptions, CacheConfig(options["cache_options"])),
			search(gameOptions, SearchConfig(options["search_options"]), tree, cache, queue),
			name(name),
			simulations(options["simulations"])
	{
	}
	void Player::setSign(Sign s) noexcept
	{
		sign = s;
	}
	Sign Player::getSign() const noexcept
	{
		return sign;
	}
	std::string Player::getName() const
	{
		return name;
	}
	void Player::prepareSearch(const matrix<Sign> &board, Move lastMove)
	{
		cache.cleanup(board);
		cache.remove(board, invertSign(lastMove.sign)); // remove current game state from cache to force neural network evaluation
		tree.clear();
		tree.getRootNode().setMove(lastMove);
		search.cleanup();
		search.setBoard(board);
	}
	bool Player::performSearch()
	{
		search.handleEvaluation(); //first handle scheduled requests
		if (search.getSimulationCount() >= simulations or tree.isProven())
			return false;
		else
		{
			search.simulate(simulations); //then perform search and schedule more requests
			return true;
		}
	}
	void Player::scheduleSingleRequest(EvaluationRequest &request)
	{
		queue.addToQueue(request);
	}
	Move Player::getMove() const noexcept
	{
		matrix<float> policy(game_config.rows, game_config.cols);
		tree.getPlayoutDistribution(tree.getRootNode(), policy);
		normalize(policy);

		Move move = pickMove(policy);
		move.sign = invertSign(Move::getSign(tree.getRootNode().getMove()));
		return move;
	}

	EvaluationGame::EvaluationGame(GameConfig gameConfig, GameBuffer &gameBuffer, bool useOpening) :
			game_buffer(gameBuffer),
			game(gameConfig),
			request(game.rows(), game.cols()),
			use_opening(useOpening)
	{
	}
	void EvaluationGame::clear()
	{
		first_player.reset();
		second_player.reset();
		state = GAME_NOT_STARTED;
		is_request_scheduled = false;
		opening_trials = 0;
		has_stored_opening = false;
	}
	void EvaluationGame::setFirstPlayer(const Json &options, EvaluationQueue &queue, const std::string &name)
	{
		first_player = std::make_unique<Player>(game.getConfig(), options, queue, name);
	}
	void EvaluationGame::setSecondPlayer(const Json &options, EvaluationQueue &queue, const std::string &name)
	{
		second_player = std::make_unique<Player>(game.getConfig(), options, queue, name);
	}
	bool EvaluationGame::prepareOpening()
	{
		assert(first_player != nullptr);

		if (use_opening)
		{
			if (has_stored_opening)
			{
				has_stored_opening = false;
				first_player->setSign(Sign::CIRCLE);
				second_player->setSign(Sign::CROSS);
				game.setPlayers(second_player->getName(), first_player->getName());
				game.loadOpening(opening);
				return true;
			}

			if (is_request_scheduled == true)
			{
				if (request.is_ready == false)
					return false;
				is_request_scheduled = false;
				if (fabsf((request.getValue().win + 0.5f * request.getValue().draw) - 0.5f) < (0.05f + opening_trials * 0.01f))
				{
					opening_trials = 0;
					has_stored_opening = true;
					first_player->setSign(Sign::CROSS);
					second_player->setSign(Sign::CIRCLE);
					game.setPlayers(first_player->getName(), second_player->getName());
					return true;
				}
				else
					opening_trials++;
			}

			opening = ag::prepareOpening(game.getConfig(), 2);
			game.loadOpening(opening);
			request.clear();
			request.setBoard(game.getBoard());
			request.setLastMove(game.getLastMove());
			first_player->scheduleSingleRequest(request);
			is_request_scheduled = true;
			return false;
		}
		else
		{
			if (has_stored_opening == false)
			{
				has_stored_opening = true;
				first_player->setSign(Sign::CROSS);
				second_player->setSign(Sign::CIRCLE);
				game.setPlayers(first_player->getName(), second_player->getName());
			}
			else
			{
				has_stored_opening = false;
				first_player->setSign(Sign::CIRCLE);
				second_player->setSign(Sign::CROSS);
				game.setPlayers(second_player->getName(), first_player->getName());
			}
			return true;
		}
	}
	void EvaluationGame::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			if (use_opening)
				opening_trials = 0;
			state = PREPARE_OPENING;
		}

		if (state == PREPARE_OPENING)
		{
			bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
			{
				state = GAMEPLAY;
				get_player().prepareSearch(game.getBoard(), game.getLastMove());
			}
		}

		if (state == GAMEPLAY)
		{
			while (game.isOver() == false)
			{
				bool searchInProgress = get_player().performSearch();
				if (searchInProgress == true)
					return;
				else
				{
					game.makeMove(get_player().getMove());
					get_player().prepareSearch(game.getBoard(), game.getLastMove());
				}
			}
			state = SEND_RESULTS;
		}

		if (state == SEND_RESULTS)
		{
			game.resolveOutcome();
			game_buffer.addToBuffer(game);
			state = GAME_NOT_STARTED;
		}
	}
	// private
	Player& EvaluationGame::get_player() const noexcept
	{
		assert(first_player != nullptr && first_player->getSign() != Sign::NONE);
		assert(second_player != nullptr && second_player->getSign() != Sign::NONE);
		assert(first_player->getSign() == invertSign(second_player->getSign()));

		if (game.getSignToMove() == first_player->getSign())
			return *first_player;
		else
			return *second_player;
	}
} /* namespace ag */

