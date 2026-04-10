/*
 * Player.cpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/evaluation/Player.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	Player::Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name) :
			nn_evaluator(evaluator),
			game_config(gameOptions),
			final_move_selection_config(options.final_selector),
			tree(options.search_config.tree_config),
			search(gameOptions, options.search_config),
			name(name),
			simulations(options.simulations)
	{
		search.setBatchSize(options.search_config.max_batch_size);
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
	void Player::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		search.cleanup(tree);
		tree.setBoard(board, signToMove);
		search.setBoard(board, signToMove);

		const MCTSConfig &mcts_config = search.getConfig().mcts_config;
		std::unique_ptr<EdgeSelector> tmp = EdgeSelector::create(mcts_config.edge_selector_config);
		tree.setEdgeSelector(*tmp);
		tree.setEdgeGenerator(UnifiedGenerator(mcts_config.max_children, mcts_config.policy_expansion_threshold, mcts_config.policy_temperature));
	}
	void Player::selectSolveEvaluate()
	{
		search.select(tree, simulations);
		search.solve();
		const int tmp = nn_evaluator.getQueueSize();
		search.scheduleToNN(nn_evaluator);
		nn_evals += nn_evaluator.getQueueSize() - tmp;
	}
	void Player::expandBackup()
	{
		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
	}
	bool Player::isSearchOver()
	{
//		if (startsWith(getName(), "value"))
//		{
//			if (tree.getSimulationCount() > tree.getBoard().size() - Board::numberOfMoves(tree.getBoard()))
//			{
//				search.cleanup(tree);
//				nn_evals = 0;
//				return true;
//			}
//			else
//				return false;
//		}
//		if (startsWith(getName(), "policy"))
//		{
//			if (tree.getSimulationCount() >= simulations)
//			{
//				search.cleanup(tree);
//				nn_evals = 0;
//				return true;
//			}
//			else
//				return false;
//		}

// if draw probability is higher than some level we proportionally reduce the number of simulations to save time (15-20% speedup)
		const Value root_eval = tree.getInfo( { }).getValue();
		const int sims = get_simulations_for_move(root_eval.draw_rate, simulations, 50);

		if (tree.getSimulationCount() > sims or tree.isRootProven())
		{
			search.cleanup(tree);
			nn_evals = 0;
			return true;
		}
		else
			return false;
	}
	AlphaBetaSearch& Player::getSolver() noexcept
	{
		return search.getSolver();
	}
	NNEvaluator& Player::getNNEvaluator() noexcept
	{
		return nn_evaluator;
	}
	void Player::scheduleSingleTask(SearchTask &task)
	{
		nn_evaluator.addToQueue(task);
	}
	Move Player::getMove() const noexcept
	{
		std::unique_ptr<EdgeSelector> selector = EdgeSelector::create(final_move_selection_config);
		const Node root_node = tree.getInfo( { });
		Edge *edge = selector->select(&root_node);

//		std::cout << "Player : " << getName() << '\n';
//		std::cout << '\n' << "selected : " << edge->toString() << '\n';
//		std::cout << Board::toString(tree.getBoard(), true) << '\n';
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < root_node.numberOfEdges(); i++)
//			std::cout << "    " << root_node.getEdge(i).toString() << '\n';

		return edge->getMove();
	}
} /* namespace ag */

