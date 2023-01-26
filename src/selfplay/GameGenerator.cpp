/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	int contains(const std::vector<int> &v, int x)
	{
		return std::find(v.begin(), v.end(), x) != v.end();
	}
}

namespace ag
{
//	GameGenerator::GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GameBuffer &gameBuffer, NNEvaluator &evaluator) :
//			game_buffer(gameBuffer),
//			nn_evaluator(evaluator),
//			game(gameOptions),
//			request(game.getRules()),
//			tree(selfplayOptions.tree_config),
//			search(gameOptions, selfplayOptions.search_config),
//			selfplay_config(selfplayOptions),
//			use_opening(selfplayOptions.use_opening)
//	{
//	}
//	void GameGenerator::setEpoch(int epoch)
//	{
//		this->simulations_min = selfplay_config.simulations_min.getValue(epoch);
//		this->simulations_max = selfplay_config.simulations_max.getValue(epoch);
//		this->positions_skip = selfplay_config.positions_skip.getValue(epoch);
//	}
//	void GameGenerator::clearStats()
//	{
//		search.clearStats();
//		tree.clearNodeCacheStats();
//	}
//	NodeCacheStats GameGenerator::getCacheStats() const noexcept
//	{
//		return tree.getNodeCacheStats();
//	}
//	SearchStats GameGenerator::getSearchStats() const noexcept
//	{
//		return search.getStats();
//	}
//	void GameGenerator::reset()
//	{
//		state = GAME_NOT_STARTED;
//		is_request_scheduled = false;
//		perform_full_search = false;
//	}
//	void GameGenerator::generate()
//	{
//		if (state == GAME_NOT_STARTED)
//		{
//			game.beginGame();
//			tree.clear();
//			if (use_opening)
//			{
//				opening_trials = 0;
//				state = PREPARE_OPENING;
//			}
//			else
//			{
//				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
//				prepare_search(game.getBoard(), game.getSignToMove());
//			}
//		}
//
//		if (state == PREPARE_OPENING)
//		{
//			bool isReady = prepare_opening();
//			if (isReady == false)
//				return;
//			else
//			{
//				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
//				prepare_search(game.getBoard(), game.getSignToMove());
//			}
//		}
//
//		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE)
//		{
//			search.select(tree);
//			search.solve();
//			search.scheduleToNN(nn_evaluator);
//			state = GAMEPLAY_EXPAND_AND_BACKUP;
//			return;
//		}
//
//		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
//		{
//			search.generateEdges(tree);
//			search.expand(tree);
//			search.backup(tree);
//
//			if (tree.getSimulationCount() > nb_of_simulations() or tree.isProven())
//				state = HANDLE_SEARCH_END;
//			else
//			{
//				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
//				return;
//			}
//		}
//
//		if (state == HANDLE_SEARCH_END)
//		{
//			if (perform_full_search)
//			{
//
//			}
//			else
//			{
//				make_move();
//				if (game.isOver())
//				{
//					game.resolveOutcome();
//					pick_positions_for_full_search();
//					perform_full_search = true;
//
//					prepare_board_for_full_search();
//					state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
//
////					if (game.getNumberOfSamples() > 0)
////						game_buffer.addToBuffer(game);
////					state = GAME_NOT_STARTED;
//					return;
//				}
//				else
//				{
//					prepare_search(game.getBoard(), game.getSignToMove());
//					state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
//				}
//			}
//			return;
//		}
//	}
//	/*
//	 * private
//	 */
//	bool GameGenerator::prepare_opening()
//	{
//		if (is_request_scheduled == true)
//		{
//			if (request.isReadyNetwork() == false)
//				return false;
//			is_request_scheduled = false;
//			if (fabsf(request.getValue().win - request.getValue().loss) < (0.05f + opening_trials * 0.01f))
//			{
//				opening_trials = 0;
//				return true;
//			}
//			else
//				opening_trials++;
//		}
//
//		std::vector<Move> opening = ag::prepareOpening(game.getConfig());
//		game.loadOpening(opening);
//		request.set(game.getBoard(), game.getSignToMove());
//		nn_evaluator.addToQueue(request);
//		is_request_scheduled = true;
//		return false;
//	}
//	void GameGenerator::make_move()
//	{
//		const Node root_node = tree.getInfo( { });
//		std::cout << "after search\n";
//		std::cout << tree.getMemory() / 1024 << "kB\n";
//		std::cout << tree.getNodeCacheStats().toString() << '\n';
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < std::min(10, root_node.numberOfEdges()); i++)
//			std::cout << "  " << root_node.getEdge(i).toString() << '\n';
//
//		matrix<float> policy(game.rows(), game.cols());
//		matrix<ProvenValue> proven_values(game.rows(), game.cols());
//		matrix<Value> action_values(game.rows(), game.cols());
//		for (int i = 0; i < root_node.numberOfEdges(); i++)
//		{
//			Move m = root_node.getEdge(i).getMove();
//			policy.at(m.row, m.col) = root_node.getEdge(i).getVisits();
//			proven_values.at(m.row, m.col) = root_node.getEdge(i).getProvenValue();
//			action_values.at(m.row, m.col) = root_node.getEdge(i).getValue();
//		}
//		normalize(policy);
//
//		BestEdgeSelector selector;
//		Edge *edge = selector.select(&root_node);
//		Move move = edge->getMove();
//
//		SearchData state(game.rows(), game.cols());
//		state.setBoard(game.getBoard());
//		state.setActionProvenValues(proven_values);
//		state.setPolicy(policy);
//		state.setActionValues(action_values);
//		state.setMinimaxValue(root_node.getValue());
//		state.setProvenValue(root_node.getProvenValue());
//		state.setMove(move);
//
//		state.print();
//
//		game.addSearchData(state);
//		game.makeMove(move);
//	}
//	void GameGenerator::prepare_search(const matrix<Sign> &board, Sign signToMove)
//	{
//		search.cleanup(tree);
//		if (perform_full_search)
//			tree.setBoard(board, signToMove, true); // force remove root node
//		else
//			tree.setBoard(board, signToMove, false); // root node can be left in the tree
//
//		tree.setEdgeSelector(PuctSelector(search.getConfig().exploration_constant, 0.5f));
//
//		SolverGenerator base_generator(search.getConfig().expansion_prior_treshold, search.getConfig().max_children);
//		if (perform_full_search)
//			tree.setEdgeGenerator(NoisyGenerator(getNoiseMatrix(board), search.getConfig().noise_weight, base_generator));
//		else
//			tree.setEdgeGenerator(base_generator);
//	}
//	int GameGenerator::nb_of_simulations() const noexcept
//	{
//		if (perform_full_search)
//			return simulations_max;
//		else
//			return simulations_min;
//	}
//	bool GameGenerator::is_tree_proven() const noexcept
//	{
//		const Node root_node = tree.getInfo(std::vector<Move>());
//		return std::all_of(root_node.begin(), root_node.end(), [](const Edge &edge)
//		{	return edge.isProven();});
//	}
//	void GameGenerator::pick_positions_for_full_search()
//	{
//		const int start = game.length() - game.getNumberOfSamples();
//		int stop = game.getNumberOfSamples();
//		for (int i = 0; i < game.getNumberOfSamples(); i++)
//			if (game.getSample(i).getProvenValue() == ProvenValue::WIN)
//			{ // found first sample where forced win was calculated
//				stop = start + i;
//				break;
//			}
//
//		const int positions = game.getNumberOfSamples() / positions_skip;
//		const std::vector<int> &game_length_stats = game_buffer.getGameLengthStats();
//
//		std::vector<int> tmp;
//		for (int i = 0; i < positions; i++)
//		{
//			int min_value = std::numeric_limits<int>::max();
//			int idx = -1;
//			for (int j = start; j < stop; j++)
//				if (game_length_stats[j] < min_value and not contains(tmp, j))
//				{
//					min_value = game_length_stats[j];
//					idx = j;
//				}
//			if (idx == -1)
//				break;
//			tmp.push_back(idx);
//		}
//		game_buffer.updateGameLengthStats(tmp);
//
//		boards_for_full_search.clear();
//		for (size_t i = 0; i < tmp.size(); i++)
//			boards_for_full_search.push_back(game.getSample(tmp[i] - start));
//	}
//	void GameGenerator::prepare_board_for_full_search()
//	{
//		assert(boards_for_full_search.size() > 0);
//		matrix<Sign> board(game.rows(), game.cols());
//
//		boards_for_full_search.back().getBoard(board);
//		const Sign sign_to_move = boards_for_full_search.back().getMove().sign;
//
//		prepare_search(board, sign_to_move);
//		boards_for_full_search.pop_back();
//	}

	GameGenerator::GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GameBuffer &gameBuffer, NNEvaluator &evaluator) :
			game_buffer(gameBuffer),
			nn_evaluator(evaluator),
			game(gameOptions),
			request(game.getRules()),
			tree(selfplayOptions.tree_config),
			search(gameOptions, selfplayOptions.search_config),
			selfplay_config(selfplayOptions),
			use_opening(selfplayOptions.use_opening)
	{
	}
	void GameGenerator::setEpoch(int epoch)
	{
		this->simulations_min = selfplay_config.simulations_min.getValue(epoch);
		this->simulations_max = selfplay_config.simulations_max.getValue(epoch);
		this->positions_skip = selfplay_config.positions_skip.getValue(epoch);
	}
	void GameGenerator::clearStats()
	{
		search.clearStats();
		tree.clearNodeCacheStats();
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
		forced_win_found = false;
	}
	void GameGenerator::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			forced_win_found = false;
			game.beginGame();
			tree.clear();
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
			bool isReady = prepare_opening();
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
			search.solve();
			search.scheduleToNN(nn_evaluator);
			state = GAMEPLAY_EXPAND_AND_BACKUP;
			return;
		}

		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			search.generateEdges(tree);
			search.expand(tree);
			search.backup(tree);

			if (tree.getSimulationCount() > nb_of_simulations() or tree.isProven())
			{
				make_move();
				if (game.isOver())
				{
					game.resolveOutcome();
					if (game.getNumberOfSamples() > 0)
						game_buffer.addToBuffer(game);
					state = GAME_NOT_STARTED;
					return;
				}
				else
					prepare_search(game.getBoard(), game.getSignToMove());
			}
			state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
			return;
		}
	}
	/*
	 * private
	 */
	bool GameGenerator::prepare_opening()
	{
		if (is_request_scheduled == true)
		{
			if (request.isReadyNetwork() == false)
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
		request.set(game.getBoard(), game.getSignToMove());
		nn_evaluator.addToQueue(request);
		is_request_scheduled = true;
		return false;
	}
	void GameGenerator::make_move()
	{
		const Node root_node = tree.getInfo( { });
//		std::cout << "after search\n";
//		std::cout << tree.getMemory() / 1024 << "kB\n";
//		std::cout << tree.getNodeCacheStats().toString() << '\n';
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < std::min(10, root_node.numberOfEdges()); i++)
//			std::cout << "  " << root_node.getEdge(i).toString() << '\n';

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

		BestEdgeSelector selector;
		Edge *edge = selector.select(&root_node);
		Move move = edge->getMove();

		SearchData state(policy.rows(), policy.cols());
		state.setBoard(game.getBoard());
		state.setActionProvenValues(proven_values);
		state.setPolicy(policy);
		state.setActionValues(action_values);
		state.setMinimaxValue(root_node.getValue());
		state.setProvenValue(root_node.getProvenValue());
		state.setMove(move);

//		state.print();

		if (perform_full_search)
			game.addSearchData(state);

		game.makeMove(move);
		if (root_node.getProvenValue() == ProvenValue::WIN and not forced_win_found)
			forced_win_found = true;
	}
	void GameGenerator::prepare_search(const matrix<Sign> &board, Sign signToMove)
	{
		perform_full_search = (randInt(positions_skip) == 0) and not forced_win_found;

		search.cleanup(tree);
		if (perform_full_search)
			tree.setBoard(board, signToMove, true); // force remove root node
		else
			tree.setBoard(board, signToMove, false); // root node can be left in the tree

		tree.setEdgeSelector(PUCTSelector(search.getConfig().exploration_constant, 0.5f));

		SolverGenerator base_generator(search.getConfig().expansion_prior_treshold, search.getConfig().max_children);
		if (perform_full_search)
			tree.setEdgeGenerator(NoisyGenerator(getNoiseMatrix(board), search.getConfig().noise_weight, base_generator));
		else
			tree.setEdgeGenerator(base_generator);
	}
	int GameGenerator::nb_of_simulations() const noexcept
	{
		if (perform_full_search)
			return simulations_max;
		else
			return simulations_min;
	}

} /* namespace ag */

