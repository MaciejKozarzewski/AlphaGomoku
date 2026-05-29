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
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;

	MovesLeftEstimator get_20x20_estimator()
	{
		std::vector<std::pair<int, float>> c0;
		c0.push_back( { 0, 60 });
		c0.push_back( { 20, 53 });
		c0.push_back( { 350, 50 });
		c0.push_back( { 400, 0 });

		std::vector<std::pair<int, float>> c2;
		c2.push_back( { 0, 200 });
		c2.push_back( { 20, 180 });
		c2.push_back( { 349, 180 });
		c2.push_back( { 350, 0 });

		return ag::MovesLeftEstimator(c0, c2);
	}
	MovesLeftEstimator get_15x15_estimator()
	{
		std::vector<std::pair<int, float>> c0;
		c0.push_back( { 0, 85 });
		c0.push_back( { 15, 85 });
		c0.push_back( { 65, 135 });
		c0.push_back( { 80, 135 });
		c0.push_back( { 100, 125 });
		c0.push_back( { 225, 0 });

		std::vector<std::pair<int, float>> c2;
		c2.push_back( { 0, 320 });
		c2.push_back( { 20, 320 });
		c2.push_back( { 65, 525 });
		c2.push_back( { 80, 525 });
		c2.push_back( { 125, 375 });
		c2.push_back( { 140, 0 });

		return ag::MovesLeftEstimator(c0, c2);
	}

	double get_time_fraction_based_on_board_size(const GameConfig &game_config, const SearchConfig &search_config)
	{
		return (game_config.rows == 15) ? search_config.time_fraction_15x15 : search_config.time_fraction_20x20;
	}
}

namespace ag
{
	Player::Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name) :
			nn_evaluator(evaluator),
			game_config(gameOptions),
			final_move_selection_config(options.final_selector),
			tree(options.search_config.tree_config),
			search(gameOptions, options.search_config),
			name(name),
			constraints(options.constraints)
	{
		search.setBatchSize(options.search_config.max_batch_size);
		if (gameOptions.rows == 20)
			moves_left_estimator = get_20x20_estimator();
		else
		{
			assert(gameOptions.rows == 15);
			moves_left_estimator = get_15x15_estimator();
		}
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
	void Player::newGame()
	{
		time_controller.new_game();
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
		const double t0 = getTime();
		search.select(tree, constraints.max_simulations);
		search.solve();
		const int tmp = nn_evaluator.getQueueSize();
		search.scheduleToNN(nn_evaluator);
		const int nn_evals = nn_evaluator.getQueueSize() - tmp;
		const double t1 = getTime();
		time_controller.add_time(t1 - t0);
		time_controller.add_time(nn_evals / 430.211);
	}
	void Player::expandBackup()
	{
		const double t0 = getTime();
		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
		const double t1 = getTime();
		time_controller.add_time(t1 - t0);
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

		if (tree.isRootProven())
			return true;

		if (constraints.type == Constraints::Type::SIMULATIONS)
		{
			// if draw probability is higher than some level we proportionally reduce the number of simulations to save time (15-20% speedup)
			const Value root_eval = tree.getInfo( { }).getValue();
			const int sims = get_simulations_for_move(root_eval.draw_rate, constraints.max_simulations, 50);
			return tree.getSimulationCount() > sims;
		}

		if (constraints.type == Constraints::Type::TIME)
		{
			const double protocol_overhead = 0.1;
			const double time_fraction = get_time_fraction_based_on_board_size(game_config, search.getConfig());
			const double early_stopping = search.getConfig().early_stopping;

			Node root_node = tree.getInfo( { });

			if (root_node.getVisits() <= 1)
				return false;

			int most_visits = 0;
			int total_visits = 0;
			for (Edge *edge = root_node.begin(); edge < root_node.end(); edge++)
			{
				const int v = std::max(1, edge->getVisits());
				total_visits += v;
				most_visits = std::max(most_visits, v);
			}
			if (most_visits > early_stopping * total_visits)
				return true; // early stopping

			const double time_left = constraints.time_for_match - time_controller.time_match();
//			const double moves_left = moves_left_estimator.get(tree.getMoveNumber(), tree.getEvaluation());
			const double moves_left = tree.getMovesLeft();
			const double sum = (1.0 - std::pow(time_fraction, moves_left)) / (1.0 - time_fraction);
			const double search_time = std::min(constraints.time_for_turn, (time_left / sum)) - protocol_overhead;

			return time_controller.time_turn() >= search_time;
		}

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
	Move Player::getMove()
	{
		search.cleanup(tree);
		std::unique_ptr<EdgeSelector> selector = EdgeSelector::create(final_move_selection_config);
		const Node root_node = tree.getInfo( { });
		const Edge *edge = selector->select(&root_node);
		const Move move = edge->getMove();

//		std::cout << "\n\n";
//		std::cout << "---------------------------------------------------------------------------------------------------\n";
//		std::cout << "Player : " << getName() << '\n';
//		std::cout << "turn  : " << time_controller.time_turn() << " / " << constraints.time_for_turn << '\n';
//		std::cout << "match : " << time_controller.time_match() << " / " << constraints.time_for_match << '\n';
//		std::cout << '\n' << "selected : " << edge->toString() << '\n';
//		std::cout << Board::toString(tree.getBoard(), true) << '\n';
//		std::cout << root_node.toString() << '\n';
//		root_node.sortEdges();
//		for (int i = 0; i < std::min(10, root_node.numberOfEdges()); i++)
//			std::cout << "    " << root_node.getEdge(i).toString() << '\n';
//		if (root_node.numberOfEdges() > 10)
//			std::cout << "    ... (" << (root_node.numberOfEdges() - 10) << " edges omitted)\n";
//		std::cout << "---------------------------------------------------------------------------------------------------\n";

		time_controller.new_turn();
		return move;
	}
} /* namespace ag */

