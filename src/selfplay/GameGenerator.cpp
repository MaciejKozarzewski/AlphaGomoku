/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	GameGenerator::GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GameBuffer &gameBuffer, NNEvaluator &evaluator) :
			game_buffer(gameBuffer),
			nn_evaluator(evaluator),
			game(gameOptions),
			request(game.getRules()),
			tree(selfplayOptions.tree_config),
			search(gameOptions, selfplayOptions.search_config),
			simulations(selfplayOptions.simulations),
			temperature(selfplayOptions.temperature),
			use_opening(selfplayOptions.use_opening),
			search_config(selfplayOptions.search_config)
	{
	}
	void GameGenerator::clearStats()
	{
		search.clearStats();
		tree.clearTreeStats();
		tree.clearNodeCacheStats();
	}
	TreeStats GameGenerator::getTreeStats() const noexcept
	{
		return tree.getTreeStats();
	}
	NodeCacheStats GameGenerator::getCacheStats() const noexcept
	{
		return tree.getNodeCacheStats();
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
			if (request.isReady() == false)
				return false;
			is_request_scheduled = false;
			if (fabsf(request.getValue().win - request.getValue().loss) < (0.05f + opening_trials * 0.01f))
			{
				opening_trials = 0;
				return true;
			}
			else
				opening_trials++;
		}

		std::vector<Move> opening = ag::prepareOpening(game.getConfig());
		game.loadOpening(opening);
		request.reset(game.getBoard(), game.getSignToMove());
		nn_evaluator.addToQueue(request);
		is_request_scheduled = true;
		return false;
	}
	void GameGenerator::makeMove()
	{
		Node root_node = tree.getInfo( { });

		matrix<float> policy(game.rows(), game.cols());
		matrix<ProvenValue> proven_values(game.rows(), game.cols());
		matrix<Value> action_values(game.rows(), game.cols());
		for (int i = 0; i < root_node.numberOfEdges(); i++)
		{
			Move m = root_node.getEdge(i).getMove();
			policy.at(m.row, m.col) = root_node.getEdge(i).getVisits();
			proven_values.at(m.row, m.col) = root_node.getEdge(i).getProvenValue();
			action_values.at(m.row, m.col) = root_node.getEdge(i).getValue();
		}
		normalize(policy);

		Move move;
		if (temperature == 0.0f)
			move = pickMove(policy);
		else
			move = randomizeMove(policy, temperature);
		move.sign = root_node.getSignToMove();

		SearchData state(policy.rows(), policy.cols());
		state.setBoard(game.getBoard());
		state.setActionProvenValues(proven_values);
		state.setPolicy(policy);
		state.setActionValues(action_values);
		state.setMinimaxValue(root_node.getValue());
		state.setProvenValue(root_node.getProvenValue());
		state.setMove(move);

		game.makeMove(move);
		game.addSearchData(state);
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
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				prepare_search(game.getBoard(), game.getSignToMove());
			}
		}

		if (state == PREPARE_OPENING)
		{
			bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
			{
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				prepare_search(game.getBoard(), game.getSignToMove());
			}
		}

		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE)
		{
			search.select(tree);
			search.tryToSolve();
			search.scheduleToNN(nn_evaluator);
			state = GAMEPLAY_EXPAND_AND_BACKUP;
			return;
		}

		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			search.generateEdges(tree);
			search.expand(tree);
			search.backup(tree);

			if (tree.getSimulationCount() > simulations or tree.isProven())
			{
				makeMove();
				if (game.isOver())
					state = SEND_RESULTS;
				else
				{
					prepare_search(game.getBoard(), game.getSignToMove());
					state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				}
			}
		}

		if (state == SEND_RESULTS)
		{
			game.resolveOutcome();
			game_buffer.addToBuffer(game);
			state = GAME_NOT_STARTED;
		}
	}
	/*
	 * private
	 */
	void GameGenerator::prepare_search(const matrix<Sign> &board, Sign signToMove)
	{
		search.cleanup(tree);
		tree.setBoard(board, signToMove, true);
		tree.setEdgeSelector(PuctSelector(search_config.exploration_constant, 0.5f));

		SolverGenerator base_generator(search_config.expansion_prior_treshold, search_config.max_children);
		tree.setEdgeGenerator(NoisyGenerator(getNoiseMatrix(board), search_config.noise_weight, base_generator));
	}

} /* namespace ag */

