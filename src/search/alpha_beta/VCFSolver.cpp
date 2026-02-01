/*
 * VCFSolver.cpp
 *
 *  Created on: Aug 19, 2025
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/VCFSolver.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>

#include <minml/graph/Graph.hpp>

#include <iostream>
#include <cassert>

namespace
{
	using namespace ag;

	class ScopedCounter
	{
			std::string name;
			size_t counter = 0;
		public:
			ScopedCounter(const std::string &name) :
					name(name)
			{
			}
			~ScopedCounter()
			{
				std::cout << "ScopedCounter \"" << name << "\' = " << counter << '\n';
			}
			void increment() noexcept
			{
				counter++;
			}
	};

	int get_max_nodes(GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size + 1) / 2;
	}

	bool contains_edge(const SearchTask &task, Move m) noexcept
	{
		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			if (edge->getMove() == m)
				return true;
		return false;
	}

}

namespace ag
{

	VCFSolver::VCFSolver(const GameConfig &gameConfig) :
			action_stack(get_max_nodes(gameConfig)),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			move_generator(gameConfig, pattern_calculator),
//			policy_nnue(gameConfig, 1, "nnue_policy_s2_5x5_32x32x1.bin"),
			policy(gameConfig.rows, gameConfig.cols),
			action_values(gameConfig.rows, gameConfig.cols),
			shared_table(gameConfig.rows, gameConfig.cols, 4 * 1024 * 1024),
			total_time("total_time"),
			policy_time("policy_time")
	{
		network = loadAGNetwork("/home/maciek/alphagomoku/final_runs_2025/fast_policy_3x3/network_swa_opt.bin");
		network->setBatchSize(1);
//		loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
	}
	void VCFSolver::increaseGeneration()
	{
		shared_table.increaseGeneration();
	}
	void VCFSolver::clear()
	{
		shared_table.clear();
	}
	void VCFSolver::loadWeights(const nnue::NNUEWeights &weights)
	{
		inference_nnue = nnue::InferenceNNUE(game_config, weights);
	}
	int VCFSolver::solve(SearchTask &task)
	{
		TimerGuard tg(total_time);

		start_time = getTime();

		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());
		task.getFeatures().encode(pattern_calculator);
//		inference_nnue.refresh(pattern_calculator);

		node_counter = 0;

		ActionList actions(action_stack);
		Score result;
		hash_key = shared_table.getHashFunction().getHash(task.getBoard()); // set up hash key
		for (int depth = 0; depth <= -max_depth; depth += 4)
		{ // iterative deepening loop
//			double t0 = getTime();
			const size_t max_stack_offset = action_stack.max_offset();
			result = solve_attack(depth, actions);
//			std::cout << "max depth=" << depth << ", nodes=" << node_counter << ", stack offset=" << action_stack.max_offset() << ", score=" << result
//					<< ", time=" << (getTime() - t0) << "s\n\n\n\n";
//			actions.print();
//			std::cout << '\n';

			/*
			 * We can stop the search if:
			 * - no actions were generated, or
			 * - we have proven score, or
			 * - we have exceeded the max number of nodes, or
			 * - no new nodes were added to the tree
			 * - there is no time left
			 */
			if (actions.isEmpty() or result.isProven() or node_counter >= max_nodes or action_stack.max_offset() == max_stack_offset
					or (getTime() - start_time) >= max_time)
				break;
		}
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			task.getActionScores().at(m.row, m.col) = iter->score;
			if (task.getActionScores().at(m.row, m.col).isProven())
				task.getActionValues().at(m.row, m.col) = task.getActionScores().at(m.row, m.col).convertToValue();
			task.addEdge(m);
		}
		task.setScore(result);
		if (task.getScore().isProven())
		{
			task.setValue(task.getScore().convertToValue());
			task.setMovesLeft(task.getScore().getDistance());
			task.setValueUncertainty(0.0f);
		}
		if (actions.must_defend)
			task.markAsDefensive();
		task.markAsProcessedBySolver();

		actions = ActionList(action_stack);
		const Score static_score = move_generator.generate(actions, MoveGeneratorMode::OPTIMAL);

		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			if (not contains_edge(task, m))
			{
				task.getActionScores().at(m.row, m.col) = iter->score;
				if (task.getActionScores().at(m.row, m.col).isProven())
					task.getActionValues().at(m.row, m.col) = task.getActionScores().at(m.row, m.col).convertToValue();
				task.addEdge(m);
			}
		}

//		if (task.getScore().isProven())
//		{
//			std::cout << result.toString() << " at " << node_counter << '\n';
//			std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//			pattern_calculator.print();
//			pattern_calculator.printAllThreats();
//			pattern_calculator.printForbiddenMoves();
//			std::sort(actions.begin(), actions.end());
//			actions.print();
//			std::cout << task.toString();
//			std::cout << "\n---------------------------------------------\n";
//		}
		total_positions += node_counter;
		total_calls++;

//		print_stats();
		return node_counter;
	}
	void VCFSolver::print_stats() const
	{
		pattern_calculator.print_stats();
		std::cout << total_time.toString() << '\n';
		std::cout << policy_time.toString() << '\n';
		std::cout << "total positions = " << total_positions << " : " << total_positions / total_calls << "\n";
		std::cout << "SharedHashTable load factor = " << shared_table.loadFactor(true) << '\n';
		inference_nnue.print_stats();
	}
	int64_t VCFSolver::getMemory() const noexcept
	{
		return action_stack.size() * sizeof(Action);
	}
	void VCFSolver::setDepthLimit(int depth) noexcept
	{
		max_depth = depth;
	}
	void VCFSolver::setNodeLimit(int nodes) noexcept
	{
		max_nodes = nodes;
	}
	void VCFSolver::setTimeLimit(double time) noexcept
	{
		max_time = time;
	}
	/*
	 * private
	 */
	Score VCFSolver::solve_attack(int depthRemaining, ActionList &actions)
	{
		{ // lookup to the shared hash table
			const SharedTableData tt_entry = shared_table.seek(hash_key);
			const Score tt_score = tt_entry.score();
			if (tt_entry.score().isProven())
				return tt_score;
		}

		node_counter++;

		const bool newly_created_node = actions.isEmpty();
		if (actions.isEmpty())
		{
			const Score static_score = move_generator.generate(actions, MoveGeneratorMode::THREATS);

			if (static_score.isProven())
				return static_score;
		}

		if (depthRemaining <= 0)
			return Score();

		if (not actions.has_initiative)
			return Score();

		if (newly_created_node)
		{
			network->packInputData(0, pattern_calculator.getBoard(), pattern_calculator.getSignToMove());
			network->forward(1);
			Value value;
			float moves_left;
			network->unpackOutput(0, policy, action_values, value, moves_left);

			for (int i = 0; i < actions.size(); i++)
				if (actions[i].score.isUnproven())
				{
					const Move move = actions[i].move;
					actions[i].score = Score(1000 * policy.at(move.row, move.col));
				}
		}

		Score best_score = Score::minus_infinity();
		Move best_move;
		for (int i = 0; i < actions.size(); i++)
		{
			// apply move ordering by selecting best action from the remaining ones
//			if (i == 0 and is_move_legal(best_move))
//				actions.moveCloserToFront(best_move, 0);
//			else
			{
				int idx = i;
				for (int j = i + 1; j < actions.size(); j++)
					if (actions[idx] < actions[j])
						idx = j;
				std::swap(actions[i], actions[idx]);
			}

			if (actions[i].score.isUnproven() and node_counter < max_nodes and (getTime() - start_time) < max_time)
			{
				const Move move = actions[i].move;
				shared_table.getHashFunction().updateHash(hash_key, move);
				shared_table.prefetch(hash_key);

				ActionList next_ply_actions(action_stack, actions, i);

				pattern_calculator.addMove(move);
				actions[i].score = invert_up(solve_defend(depthRemaining - 1, next_ply_actions));
				pattern_calculator.undoMove(move);
				shared_table.getHashFunction().updateHash(hash_key, move);
			}
			if (actions[i].score > best_score)
			{
				best_score = actions[i].score;
				best_move = actions[i].move;
			}

			if (actions[i].score.isWin())
				break;
		}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but the state is not fully expanded (we cannot prove a loss)
		// we need to override the score with something, for example with evaluation
		if (actions.size() == 0 or (best_score.isLoss() and not actions.is_fully_expanded))
			best_score = Score();
		assert(best_score != Score::minus_infinity());

		if (best_score.isProven())
		{
			const SharedTableData entry(Bound::EXACT, depthRemaining, best_score, best_move);
			shared_table.insert(hash_key, entry);
		}

		return best_score;
	}
	Score VCFSolver::solve_defend(int depthRemaining, ActionList &actions)
	{
		{ // lookup to the shared hash table
			const SharedTableData tt_entry = shared_table.seek(hash_key);
			const Score tt_score = tt_entry.score();
			if (tt_entry.score().isProven())
				return tt_score;
		}

		node_counter++;

		const bool newly_created_node = actions.isEmpty();
		if (actions.isEmpty())
		{
			const Score static_score = move_generator.generate(actions, MoveGeneratorMode::THREATS);

			if (static_score.isProven())
				return static_score;
		}

		if (depthRemaining <= 0)
			return Score();

		if (not actions.must_defend)
			return Score();

		if (newly_created_node)
		{
			network->packInputData(0, pattern_calculator.getBoard(), pattern_calculator.getSignToMove());
			network->forward(1);
			Value value;
			float moves_left;
			network->unpackOutput(0, policy, action_values, value, moves_left);

			for (int i = 0; i < actions.size(); i++)
				if (actions[i].score.isUnproven())
				{
					const Move move = actions[i].move;
					actions[i].score = Score(1000 * policy.at(move.row, move.col));
				}
		}

		Score best_score = Score::minus_infinity();
		Move best_move;
		for (int i = 0; i < actions.size(); i++)
		{
			// apply move ordering by selecting best action from the remaining ones
//			if (i == 0 and is_move_legal(best_move))
//				actions.moveCloserToFront(best_move, 0);
//			else
			{
				int idx = i;
				for (int j = i + 1; j < actions.size(); j++)
					if (actions[idx] < actions[j])
						idx = j;
				std::swap(actions[i], actions[idx]);
			}

			if (actions[i].score.isUnproven() and node_counter < max_nodes and (getTime() - start_time) < max_time)
			{
				const Move move = actions[i].move;
				shared_table.getHashFunction().updateHash(hash_key, move);
				shared_table.prefetch(hash_key);

				ActionList next_ply_actions(action_stack, actions, i);

				pattern_calculator.addMove(move);
				actions[i].score = invert_up(solve_attack(depthRemaining - 1, next_ply_actions));
				pattern_calculator.undoMove(move);
				shared_table.getHashFunction().updateHash(hash_key, move);
			}
			if (actions[i].score > best_score)
			{
				best_score = actions[i].score;
				best_move = actions[i].move;
			}

			if (actions[i].score.isWin())
				break;
		}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but the state is not fully expanded (we cannot prove a loss)
		// we need to override the score with something, for example with evaluation
		if (actions.size() == 0 or (best_score.isLoss() and not actions.is_fully_expanded))
			best_score = Score();
		assert(best_score != Score::minus_infinity());

		if (best_score.isProven())
		{
			const SharedTableData entry(Bound::EXACT, depthRemaining, best_score, best_move);
			shared_table.insert(hash_key, entry);
		}

		return best_score;
	}
	bool VCFSolver::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

} /* namespace ag */

