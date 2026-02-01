/*
 * AlphaBetaSearch.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/AlphaBetaSearch.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <minml/graph/Graph.hpp>
#include <alphagomoku/utils/file_util.hpp>

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

}

namespace ag
{

	AlphaBetaSearch::AlphaBetaSearch(const GameConfig &gameConfig) :
			action_stack(get_max_nodes(gameConfig)),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			move_generator(gameConfig, pattern_calculator),
//			policy_nnue(gameConfig, 1, "nnue_policy_s2_5x5_32x32x1.bin"),
			shared_table(gameConfig.rows, gameConfig.cols, 4 * 1024 * 1024),
			total_time("total_time"),
			policy_time("policy_time")
	{
//		loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
	}
	void AlphaBetaSearch::increaseGeneration()
	{
		shared_table.increaseGeneration();
	}
	void AlphaBetaSearch::clear()
	{
		shared_table.clear();
	}
	void AlphaBetaSearch::loadWeights(const nnue::NNUEWeights &weights)
	{
		inference_nnue = nnue::InferenceNNUE(game_config, weights);
	}
	int AlphaBetaSearch::solve(SearchTask &task)
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
		for (int depth = 0; depth <= max_depth; depth += 4)
		{ // iterative deepening loop
//			double t0 = getTime();
			const size_t max_stack_offset = action_stack.max_offset();
			result = recursive_solve(depth, Score::min_value(), Score::max_value(), actions);
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

//		if (result.score.getEval() ==  Score::min_value())
//		if (task.getScore().isProven())
//		{
//		std::cout << result.toString() << " at " << node_counter << '\n';
//		std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//		pattern_calculator.print();
//		pattern_calculator.printAllThreats();
//		pattern_calculator.printForbiddenMoves();
//		std::sort(actions.begin(), actions.end());
//		actions.print();
//		std::cout << task.toString();
//		std::cout << "\n---------------------------------------------\n";
//		exit(-1);
//		}
		total_positions += node_counter;
		total_calls++;

//		print_stats();
		return node_counter;
	}
	void AlphaBetaSearch::print_stats() const
	{
		pattern_calculator.print_stats();
		std::cout << total_time.toString() << '\n';
		std::cout << policy_time.toString() << '\n';
		std::cout << "total positions = " << total_positions << " : " << total_positions / total_calls << "\n";
		std::cout << "SharedHashTable load factor = " << shared_table.loadFactor(true) << '\n';
		inference_nnue.print_stats();
	}
	int64_t AlphaBetaSearch::getMemory() const noexcept
	{
		return action_stack.size() * sizeof(Action);
	}
	void AlphaBetaSearch::setDepthLimit(int depth) noexcept
	{
		max_depth = depth;
	}
	void AlphaBetaSearch::setNodeLimit(int nodes) noexcept
	{
		max_nodes = nodes;
	}
	void AlphaBetaSearch::setTimeLimit(double time) noexcept
	{
		max_time = time;
	}
	/*
	 * private
	 */
	Score AlphaBetaSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions)
	{
//		if (not (alpha < beta))
//		{
//			pattern_calculator.print();
//			pattern_calculator.printAllThreats();
//			std::cout << "alpha = " << alpha << ", beta = " << beta << '\n';
//		}
//		assert(Score::minus_infinity() <= alpha);
//		assert(alpha < beta);
//		assert(beta <= Score::plus_infinity());

		Move best_move;
		{ // lookup to the shared hash table
			const SharedTableData tt_entry = shared_table.seek(hash_key);
			const Bound tt_bound = tt_entry.bound();

			if (tt_bound != Bound::NONE)
			{ // there is some information stored in this entry
				best_move = tt_entry.move();
				if (not actions.isRoot())
				{ // we must not terminate search at root with just one best action on hash table hit, as the other modules may require full list of actions
					const Score tt_score = tt_entry.score();
					if (tt_entry.score().isProven())
						return tt_score;
					if (tt_entry.depth() >= depthRemaining
							and ((tt_bound == Bound::EXACT) or (tt_bound == Bound::LOWER and tt_score >= beta)
									or (tt_bound == Bound::UPPER and tt_score <= alpha)))
						return tt_score;
				}
			}
		}

		node_counter++;

//		if (not actions.isRoot() and not actions.performed_pattern_update)
//		{
//			pattern_calculator.addMove(actions.last_move);
//			actions.performed_pattern_update = true;
//		}

//		pattern_calculator.print(actions.last_move);
//		pattern_calculator.printAllThreats();
//		std::cout << "Depth remaining = " << depthRemaining << '\n';
//		std::cout << "Actions before (stack offset = " << action_stack.offset() << ")\n";
//		actions.print();

		if (actions.isEmpty())
		{
			const MoveGeneratorMode mode = actions.isRoot() ? MoveGeneratorMode::OPTIMAL : MoveGeneratorMode::THREATS;
			const Score static_score = move_generator.generate(actions, mode);
//			std::cout << "Actions generated (stack offset = " << action_stack.offset() << ")\n";
//			actions.print();

			if (static_score.isProven())
				return static_score;
		}

//		inference_nnue.update(pattern_calculator);
		if (depthRemaining <= 0)
			return evaluate();

//		if (actions.size() > 1)
//		{
//			policy_time.startTimer();
//			policy_nnue.packInputData(0, pattern_calculator.getBoard(), pattern_calculator.getSignToMove());
//			policy_nnue.forward(1);
//			matrix<float> policy(game_config.rows, game_config.cols);
//			policy_nnue.unpackOutput(0, policy);
//			policy_time.stopTimer();
//
//			for (int i = 0; i < actions.size(); i++)
//				if (actions[i].score != Score())
//					actions[i].score = Score(static_cast<int>(1000 * policy.at(actions[i].move.row, actions[i].move.col)));
//		}

		const Score original_alpha = alpha;
		Score best_score = Score::minus_infinity();
		for (int i = 0; i < actions.size(); i++)
		{
			// apply move ordering by selecting best action from the remaining ones
			if (i == 0 and is_move_legal(best_move))
				actions.moveCloserToFront(best_move, 0);
			else
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

				// construct next ply action list
				ActionList next_ply_actions(action_stack, actions, i);

				const Score new_alpha = invert_down(alpha);
				const Score new_beta = invert_down(beta);

				pattern_calculator.addMove(move);
//				if (i == 0)
				actions[i].score = invert_up(recursive_solve(depthRemaining - 1, new_beta, new_alpha, next_ply_actions));
//				else
//				{
//					actions[i].score = invert_up(recursive_solve(depthRemaining - 1, new_alpha - 1, new_alpha, next_ply_actions));
//					if (actions[i].score > alpha)
//						actions[i].score = invert_up(recursive_solve(depthRemaining - 1, new_beta, new_alpha, next_ply_actions));
//				}
				pattern_calculator.undoMove(move);

//				if (next_ply_actions.performed_pattern_update)
//				{
//					pattern_calculator.undoMove(move);
//					inference_nnue.update(pattern_calculator);
//				}
				shared_table.getHashFunction().updateHash(hash_key, move);
			}
			best_score = std::max(best_score, actions[i].score);

			if (actions[i].score > alpha)
			{
				alpha = actions[i].score;
				best_move = actions[i].move;
			}
			if (actions[i].score >= beta or actions[i].score.isWin())
				break;
		}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but the state is not fully expanded (we cannot prove a loss)
		// we need to override the score with something, for example with evaluation
		if (actions.size() == 0 or (best_score.isLoss() and not actions.is_fully_expanded))
			best_score = evaluate();
		assert(best_score != Score::minus_infinity());

		Bound tt_bound = Bound::NONE;
		if (best_score <= original_alpha)
			tt_bound = Bound::UPPER;
		else
		{
			if (best_score >= beta)
				tt_bound = Bound::LOWER;
			else
				tt_bound = Bound::EXACT;
		}
		const SharedTableData entry(tt_bound, depthRemaining, best_score, best_move);
		shared_table.insert(hash_key, entry);

		return best_score;
	}
	bool AlphaBetaSearch::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

	Score AlphaBetaSearch::evaluate()
	{
//		return Score();

//		return Score(static_cast<int>(2000 * inference_nnue.forward() - 1000));

		const Sign own_sign = pattern_calculator.getSignToMove();
		const Sign opponent_sign = invertSign(own_sign);
		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
		const int best_threat = static_cast<int>(ThreatType::FIVE);

		static const std::array<int, 10> own_values = { 0, 0, 19, 49, 76, 170, 33, 159, 252, 0 };
		static const std::array<int, 10> opp_values = { 0, 0, -1, -50, -45, -135, -14, -154, -496, 0 };
		int result = 12;
		for (int i = worst_threat; i <= best_threat; i++)
		{
			result += own_values[i] * pattern_calculator.getThreatHistogram(own_sign).numberOf(static_cast<ThreatType>(i));
			result += opp_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).numberOf(static_cast<ThreatType>(i));
		}
		return Score(std::max(-1000, std::min(1000, result)));
	}

} /* namespace ag */

