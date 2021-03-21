/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>

namespace ag
{
	GameGenerator::GameGenerator(const GameConfig &gameConfig, GameBuffer &gameBuffer, EvaluationQueue &queue) :
			game_buffer(gameBuffer),
			queue(queue),
			game(gameConfig),
			request(gameConfig.rows, gameConfig.cols)
	{
	}

	void GameGenerator::clearStats()
	{
		assert(search != nullptr);
		search->clearStats();
	}
	SearchStats GameGenerator::getSearchStats() const
	{
		assert(search != nullptr);
		return search->getStats();
	}
	bool GameGenerator::prepareOpening()
	{
		if (is_request_scheduled == true)
		{
			if (request.is_ready == false)
				return false;
			is_request_scheduled = false;
			if (config.search_config.augment_position)
				request.augment();
			if (fabsf(request.getValue() - 0.5f) < (0.05f * (1.0f + opening_trials / 10.0f)))
			{
				game.setBoard(request.getBoard(), request.getSignToMove());
				prepare_search(game.getBoard(), game.getLastMove());
				return true;
			}
			else
				opening_trials++;
		}

		matrix<Sign> board(config.game_config.rows, config.game_config.cols);
		Sign sign_to_start = ag::prepareOpening(config.game_config.rules, board);
		request.clear();
		request.setBoard(board);
		request.setLastMove( { 0, 0, sign_to_start });
		if (config.search_config.augment_position)
			request.augment();
		queue.addToQueue(request);
		is_request_scheduled = true;
		return false;
	}
	void GameGenerator::makeMove(const matrix<float> &policy)
	{
		Move move;
		if (config.temperature == 0.0f)
			move = pickMove(policy);
		else
			move = randomizeMove(policy);
		game.makeMove(move, policy, tree->getRootNode().getValue());
		prepare_search(game.getBoard(), game.getLastMove());
	}
	bool GameGenerator::performSearch(int iterations)
	{
		search->handleEvaluation(); //first handle scheduled requests
		if (search->getSimulationCount() < iterations)
		{
			search->iterate(iterations); //then perform search and schedule more requests
			return true;
		}
		else
		{
			search->cleanup(); //might not be necessary
			return false;
		}
	}
	void GameGenerator::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			if (config.use_opening)
			{
				opening_trials = 0;
				state = PREPARE_OPENING;
			}
			else
			{
				game.beginGame(randBool() ? Sign::CROSS : Sign::CIRCLE);
				state = GAMEPLAY;
			}
		}

		if (state == PREPARE_OPENING)
		{
			bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
				state = GAMEPLAY;
		}

		if (state == GAMEPLAY)
		{
			while (game.isOver() == false)
			{
				bool searchInProgress = performSearch(config.simulations);
				if (searchInProgress == true)
					return;
				else
				{
					matrix<float> policy(config.game_config.rows, config.game_config.cols);
					tree->getPlayoutDistribution(tree->getRootNode(), policy);
					normalize(policy);
					makeMove(policy);
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
	//private
	void GameGenerator::prepare_search(const matrix<Sign> &board, Move lastMove)
	{
		cache->cleanup(board);
		tree->clear();
		tree->getRootNode().setMove(lastMove);
		search->setBoard(board);
	}

} /* namespace ag */

