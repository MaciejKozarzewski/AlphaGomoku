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

	Player::Player(GameConfig gameOptions, const Json &options, EvaluationQueue &queue) :
			queue(queue),
			game_config(gameOptions),
			tree(TreeConfig(options["tree_options"])),
			cache(gameOptions, CacheConfig(options["cache_options"])),
			search(gameOptions, SearchConfig(options["search_options"]), tree, cache, queue),
			simulations(options["simulations"])
	{
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
		{
//			matrix<float> policy(15, 15);
//			tree.getPlayoutDistribution(tree.getRootNode(), policy);
//			normalize(policy);
//			std::cout << tree.getPrincipalVariation().toString() << '\n';
//			std::cout << policyToString(search.getBoard(), policy);
//			std::cout << search.getStats().toString();
//			std::cout << queue.getStats().toString();
//			std::cout << tree.getStats().toString();
//			std::cout << cache.storedElements() << " : " << cache.bufferedElements() << " : " << cache.allocatedElements() << " : "
//					<< cache.loadFactor() << "\n\n";
			return false;
		}
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
		cross_player.reset();
		circle_player.reset();
		state = GAME_NOT_STARTED;
		is_request_scheduled = false;
		opening_trials = 0;
		has_stored_opening = false;
	}
	void EvaluationGame::setCrossPlayer(const Json &options, EvaluationQueue &queue)
	{
		cross_player = std::make_unique<Player>(game.getConfig(), options, queue);
	}
	void EvaluationGame::setCirclePlayer(const Json &options, EvaluationQueue &queue)
	{
		circle_player = std::make_unique<Player>(game.getConfig(), options, queue);
	}
	bool EvaluationGame::prepareOpening()
	{
		assert(cross_player != nullptr);
		if (has_stored_opening)
		{
			has_stored_opening = false;
			for (size_t i = 0; i < opening.size(); i++)
				opening[i].sign = invertSign(opening[i].sign);
			game.loadOpening(opening);
			return true;
		}

		if (is_request_scheduled == true)
		{
			if (request.is_ready == false)
				return false;
			is_request_scheduled = false;
			request.augment();
			if (fabsf((request.getValue().win + 0.5f * request.getValue().draw) - 0.5f) < (0.05f + opening_trials * 0.01f))
			{
				opening_trials = 0;
				has_stored_opening = true;
				return true;
			}
			else
				opening_trials++;
		}

		opening = ag::prepareOpening(game.getConfig());
		game.loadOpening(opening);
		request.clear();
		request.setBoard(game.getBoard());
		request.setLastMove(game.getLastMove());
		request.augment();
		cross_player->scheduleSingleRequest(request);
		is_request_scheduled = true;
		return false;
	}
	void EvaluationGame::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			if (use_opening)
			{
				opening_trials = 0;
				state = PREPARE_OPENING;
			}
			else
			{
				game.beginGame();
				get_player().prepareSearch(game.getBoard(), game.getLastMove());
				state = GAMEPLAY;
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
		assert(cross_player != nullptr);
		assert(circle_player != nullptr);
		if (game.getSignToMove() == Sign::CROSS)
			return *cross_player;
		else
			return *circle_player;
	}
} /* namespace ag */

