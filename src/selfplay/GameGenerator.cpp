/*
 * GameGenerator.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

namespace ag
{
	GameGenerator::GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GeneratorManager &manager,
			NNEvaluator &evaluator) :
			manager(manager),
			nn_evaluator(evaluator),
			opening_generator(gameOptions, 8),
			game(gameOptions),
			tree(selfplayOptions.search_config.tree_config),
			search(gameOptions, selfplayOptions.search_config),
			selfplay_config(selfplayOptions)
	{
		search.setBatchSize(selfplayOptions.search_config.max_batch_size);
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
	GameGenerator::Status GameGenerator::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			game.beginGame();
			game_data_storage.clear();
			tree.clear();
			search.getSolver().clear();
			if (selfplay_config.use_opening)
				state = PREPARE_OPENING;
			else
			{
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				prepare_search();
			}
		}

		if (state == PREPARE_OPENING)
		{
			if (opening_generator.isEmpty())
			{
				const OpeningGenerator::Status status = opening_generator.generate(selfplay_config.search_config.max_batch_size, nn_evaluator,
						search.getSolver());
				return (status == OpeningGenerator::OK) ? GameGenerator::OK : GameGenerator::TASKS_NOT_READY;
			}
			else
			{
				game.loadOpening(opening_generator.pop());
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				prepare_search();
			}
		}

		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE)
		{
			search.select(tree, selfplay_config.simulations);
			search.solve();
			search.scheduleToNN(nn_evaluator);
			state = GAMEPLAY_EXPAND_AND_BACKUP;
			return GameGenerator::OK;
		}

		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			if (not search.areTasksReady())
				return GameGenerator::TASKS_NOT_READY;

			search.generateEdges(tree);
			search.expand(tree);
			search.backup(tree);

			// if draw probability is higher than some level we proportionally reduce the number of simulations to save time (15-20% speedup)
			const Value root_eval = tree.getInfo( { }).getValue();
			const int simulations = get_simulations_for_move(root_eval.draw_rate, selfplay_config.simulations, 100);

			if (tree.getSimulationCount() > simulations or tree.isRootProven())
			{
				make_move();
				if (game.isOver())
				{
					const GameOutcome outcome = game.getOutcome();
					game_data_storage.setOutcome(outcome);
					game_data_storage.addMoves(game.getMoves());
					if (game_data_storage.numberOfSamples() > 0)
						manager.addToBuffer(game_data_storage);
					state = GAME_NOT_STARTED;
					return GameGenerator::OK;
				}
				else
					prepare_search();
			}
			state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
		}

		return GameGenerator::OK;
	}
	Json GameGenerator::save(SerializedObject &binary_data)
	{
		Json result = game.serialize(binary_data);
		result["state"] = static_cast<int>(state);
		result["offset"] = binary_data.size();
		game_data_storage.serialize(binary_data);
		return result;
	}
	void GameGenerator::load(const Json &json, const SerializedObject &binary_data)
	{
		state = static_cast<GameState>(json["state"].getInt());
		game = Game(json, binary_data);
		size_t offset = json["offset"].getLong();
		game_data_storage = GameDataStorage(binary_data, offset, 200);
		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE or state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			prepare_search();
			state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
		}
	}
	/*
	 * private
	 */
	void GameGenerator::make_move()
	{
//		const GameConfig game_config = game.getConfig();
		const Node root_node = tree.getInfo( { });
//		std::cout << "after search\n";
//		std::cout << tree.getMemory() / 1024 << "kB\n";
//		std::cout << tree.getNodeCacheStats().toString() << '\n';
//		std::cout << search.getStats().toString() << '\n';
//		std::cout << nn_evaluator.getStats().toString() << '\n';
//		search.getSolver().print_stats();
//		tree.clearNodeCacheStats();
//		search.clearStats();
//		std::cout << Board::toString(game.getBoard(), true);
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < std::min(20, root_node.numberOfEdges()); i++)
//		for (int i = 0; i < root_node.numberOfEdges(); i++)
//			std::cout << "    " << root_node.getEdge(i).toString() << '\n';
//		tree.printSubtree(2, false);

		std::unique_ptr<EdgeSelector> selector = EdgeSelector::create(selfplay_config.final_selector);
		const Move move = selector->select(&root_node)->getMove();

		SearchDataPack sample(root_node, game.getBoard());
//		data.print();

		game_data_storage.addSample(sample);
		game.makeMove(move);
	}
	void GameGenerator::prepare_search()
	{
		search.cleanup(tree);
		tree.setBoard(game.getBoard(), game.getSignToMove(), false); // force remove root node
		search.setBoard(game.getBoard(), game.getSignToMove());

		const MCTSConfig &mcts_config = search.getConfig().mcts_config;
		std::unique_ptr<EdgeSelector> tmp = EdgeSelector::create(mcts_config.edge_selector_config);
		tree.setEdgeSelector(*tmp);
		tree.setEdgeGenerator(UnifiedGenerator(mcts_config.max_children, mcts_config.policy_expansion_threshold, true));
	}

} /* namespace ag */

