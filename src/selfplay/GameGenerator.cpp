/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	GameGenerator::GameGenerator(const Json &options, GameBuffer &gameBuffer, EvaluationQueue &queue) :
			game_buffer(gameBuffer),
			queue(queue),
			game(GameConfig(options["game_options"])),
			request(game.rows(), game.cols()),
			tree(TreeConfig(options["selfplay_options"]["tree_options"])),
			cache(game.getConfig(), CacheConfig(options["selfplay_options"]["cache_options"])),
			search(game.getConfig(), SearchConfig(options["selfplay_options"]["search_options"]), tree, cache, queue),
			simulations(options["selfplay_options"]["simulations"]),
			temperature(options["selfplay_options"]["temperature"]),
			use_opening(options["selfplay_options"]["use_opening"])
	{
	}
	void GameGenerator::clearStats()
	{
		search.clearStats();
		tree.clearStats();
		cache.clearStats();
	}
	TreeStats GameGenerator::getTreeStats() const noexcept
	{
		return tree.getStats();
	}
	CacheStats GameGenerator::getCacheStats() const noexcept
	{
		return cache.getStats();
	}
	SearchStats GameGenerator::getSearchStats() const noexcept
	{
		return search.getStats();
	}
	void GameGenerator::reset()
	{
		state = GAME_NOT_STARTED;
		is_request_scheduled = false;
	}
	bool GameGenerator::prepareOpening()
	{
		if (is_request_scheduled == true)
		{
			if (request.is_ready == false)
				return false;
			is_request_scheduled = false;
			if (fabsf((request.getValue().win + 0.5f * request.getValue().draw) - 0.5f) < (0.05f + opening_trials * 0.01f))
			{
				opening_trials = 0;
				return true;
			}
			else
				opening_trials++;
		}

		std::vector<Move> opening = ag::prepareOpening(game.getConfig());
		game.loadOpening(opening);
		request.clear();
		request.setBoard(game.getBoard());
		request.setLastMove(game.getLastMove());
		queue.addToQueue(request);
		is_request_scheduled = true;
		return false;
	}
	void GameGenerator::makeMove()
	{
		matrix<float> policy(game.rows(), game.cols());
		tree.getPlayoutDistribution(tree.getRootNode(), policy);
		normalize(policy);

		Move move;
		if (temperature == 0.0f)
			move = pickMove(policy);
		else
			move = randomizeMove(policy, temperature);
		move.sign = invertSign(Move::getSign(tree.getRootNode().getMove()));

		matrix<ProvenValue> proven_values(game.rows(), game.cols());
		matrix<Value> action_values(game.rows(), game.cols());
		tree.getProvenValues(tree.getRootNode(), proven_values);
		tree.getActionValues(tree.getRootNode(), action_values);

		SearchData state(policy.rows(), policy.cols());
		state.setBoard(game.getBoard());
		state.setActionProvenValues(proven_values);
		state.setPolicy(policy);
		state.setActionValues(action_values);
		state.setMinimaxValue(tree.getRootNode().getValue());
		state.setProvenValue(tree.getRootNode().getProvenValue());
		state.setMove(move);

		game.makeMove(move);
		game.addSearchData(state);
	}
	bool GameGenerator::performSearch()
	{
		search.handleEvaluation(); //first handle scheduled requests
		if (search.getSimulationCount() >= simulations or tree.isProven())
		{
//			std::cout << tree.getPrincipalVariation().toString() << '\n';
//			matrix<float> policy(15, 15);
//			tree.getPolicyPriors(tree.getRootNode(), policy);
//			std::cout << policyToString(game.getBoard(), policy);
//			tree.getPlayoutDistribution(tree.getRootNode(), policy);
//			normalize(policy);
//			std::cout << policyToString(game.getBoard(), policy);
//			tree.printSubtree(tree.getRootNode(), 1, true);
			return false;
		}
		else
		{
			search.simulate(simulations); //then perform search and schedule more requests
			return true;
		}
	}
	void GameGenerator::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			game.beginGame();
			if (use_opening)
			{
				opening_trials = 0;
				state = PREPARE_OPENING;
			}
			else
			{
				state = GAMEPLAY;
				prepare_search(game.getBoard(), game.getLastMove());
			}
		}

		if (state == PREPARE_OPENING)
		{
			bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
			{
				state = GAMEPLAY;
				prepare_search(game.getBoard(), game.getLastMove());
			}
		}

		if (state == GAMEPLAY)
		{
			while (game.isOver() == false)
			{
				bool searchInProgress = performSearch();
				if (searchInProgress == true)
					return;
				else
				{
					makeMove();
					prepare_search(game.getBoard(), game.getLastMove());
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
		cache.cleanup(board);
		cache.remove(board, invertSign(lastMove.sign)); // remove current game state from cache to force neural network evaluation
		tree.clear();
		tree.getRootNode().setMove(lastMove);
		search.cleanup();
		search.setBoard(board);
	}

} /* namespace ag */

