/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
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
	GameGenerator::GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GameBuffer &gameBuffer, NNEvaluator &evaluator) :
			game_buffer(gameBuffer),
			nn_evaluator(evaluator),
			opening_generator(gameOptions, 10),
			game(gameOptions),
			tree(selfplayOptions.tree_config),
			search(gameOptions, selfplayOptions.search_config),
			selfplay_config(selfplayOptions)
	{
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
	}
	void GameGenerator::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			game.beginGame();
			tree.clear();
			if (selfplay_config.use_opening)
				state = PREPARE_OPENING;
			else
			{
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				prepare_search(game.getBoard(), game.getSignToMove());
			}
		}

		if (state == PREPARE_OPENING)
		{
			const bool isReady = prepare_opening();
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

			if (tree.getSimulationCount() > selfplay_config.simulations or tree.hasAllMovesProven())
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
		if (opening_generator.isEmpty())
			opening_generator.generate(selfplay_config.search_config.max_batch_size, nn_evaluator, search.getSolver());

		if (opening_generator.isEmpty())
			return false;
		else
		{
			game.loadOpening(opening_generator.pop());
			return true;
		}
	}
	void GameGenerator::make_move()
	{
		const GameConfig game_config = game.getConfig();
		const Node root_node = tree.getInfo( { });
//		std::cout << "after search\n";
//		std::cout << tree.getMemory() / 1024 << "kB\n";
//		std::cout << tree.getNodeCacheStats().toString() << '\n';
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < std::min(10, root_node.numberOfEdges()); i++)
//			std::cout << "  " << root_node.getEdge(i).toString() << '\n';

		BestEdgeSelector selector;
		const Move move = selector.select(&root_node)->getMove();

		SearchData state(game_config.rows, game_config.cols);
		state.setBoard(tree.getBoard());
		state.setSearchResults(root_node);
		state.setMove(move);

//		state.print();

		game.addSearchData(state);

		game.makeMove(move);
	}
	void GameGenerator::prepare_search(const matrix<Sign> &board, Sign signToMove)
	{
		search.cleanup(tree);
		tree.setBoard(board, signToMove, true); // force remove root node

		tree.setEdgeSelector(PUCTSelector(search.getConfig().exploration_constant, 0.5f));
		tree.setEdgeGenerator(BaseGenerator(search.getConfig().max_children));
	}

} /* namespace ag */

