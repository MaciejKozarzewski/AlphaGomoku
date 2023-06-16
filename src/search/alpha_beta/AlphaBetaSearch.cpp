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

	AlphaBetaSearch::AlphaBetaSearch(const GameConfig &gameConfig, MoveGeneratorMode mode) :
			generator_stack(gameConfig.rows * gameConfig.cols + 10),
			action_stack(get_max_nodes(gameConfig)),
			movegen_mode(mode),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			move_generator(gameConfig, pattern_calculator),
			shared_table(gameConfig.rows, gameConfig.cols, 4 * 1024 * 1024),
			total_time("total_time")
	{
		loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
	}
	void AlphaBetaSearch::increaseGeneration()
	{
		shared_table.increaseGeneration();
	}
	void AlphaBetaSearch::loadWeights(const nnue::NNUEWeights &weights)
	{
		inference_nnue = nnue::InferenceNNUE(game_config, weights);
	}
	int AlphaBetaSearch::solve(SearchTask &task, int maxDepth, int maxPositions)
	{
		TimerGuard tg(total_time);

		max_positions = maxPositions;
		const size_t stack_size = game_config.rows * game_config.cols * maxPositions;
		if (action_stack.size() < stack_size)
			action_stack = ActionStack(stack_size);

		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());
		task.getFeatures().encode(pattern_calculator);
		inference_nnue.refresh(pattern_calculator);

		position_counter = 0;
		move_updates.clear();

		ActionList actions = action_stack.create_root();
		Score result;
		hash_key = shared_table.getHashFunction().getHash(task.getBoard()); // set up hash key
		for (int depth = 0; depth <= maxDepth; depth += 1)
		{ // iterative deepening loop
			double t0 = getTime();
			result = recursive_solve(depth, Score::minus_infinity(), Score::plus_infinity(), actions);
			std::cout << "depth=" << depth << ", nodes=" << position_counter << ", score=" << result << ", time=" << (getTime() - t0) << "s\n";
//			actions.print();

			/*
			 * We can stop the search if:
			 * - no actions were generated, or
			 * - we have proven score, or
			 * - we have exceeded the max number of nodes, or
			 * - no new nodes were added to the tree
			 */
			if (result.isProven() or position_counter >= max_positions)
				break;
		}
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			task.getActionScores().at(m.row, m.col) = iter->score;
			if (task.getActionScores().at(m.row, m.col).isProven())
				task.getActionValues().at(m.row, m.col) = task.getActionScores().at(m.row, m.col).convertToValue();
			if (actions.must_defend)
				task.addDefensiveMove(m);
		}
		task.setScore(result);
		if (task.getScore().isProven())
			task.setValue(task.getScore().convertToValue());
		task.markAsProcessedBySolver();

//		if (result.score.getEval() ==  Score::min_value())
//		if (task.getScore().getEval() == Score::min_value())
//		{
//		std::cout << result.toString() << " at " << position_counter << '\n';
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
		total_positions += position_counter;
		total_calls++;

//		print_stats();
		return position_counter;
	}
	void AlphaBetaSearch::print_stats() const
	{
		pattern_calculator.print_stats();
		std::cout << total_time.toString() << '\n';
		std::cout << "total positions = " << total_positions << " : " << total_positions / total_calls << "\n";
		std::cout << "SharedHashTable load factor = " << shared_table.loadFactor(true) << '\n';
		inference_nnue.print_stats();
	}
	/*
	 * private
	 */
	Score AlphaBetaSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions)
	{
		assert(depthRemaining >= 0);
		assert(Score::minus_infinity() <= alpha);
		assert(alpha < beta);
		assert(beta <= Score::plus_infinity());

		Move best_move;
//		{ // lookup to the shared hash table
//			const SharedTableData tt_entry = shared_table.seek(hash_key);
//			const Bound tt_bound = tt_entry.bound();
//
//			if (tt_bound != Bound::NONE) //  and is_move_legal(tt_entry.move()))
//			{ // there is some information stored in this entry
//				best_move = tt_entry.move();
//				if (not actions.isRoot())
//				{ // we must not terminate search at root with just one best action on hash table hit, as the other modules may require full list of actions
//					const Score tt_score = tt_entry.score();
//					if (tt_entry.score().isProven())
//						return tt_score;
//					if (tt_entry.depth() >= depthRemaining
//							and ((tt_bound == Bound::EXACT) or (tt_bound == Bound::LOWER and tt_score >= beta)
//									or (tt_bound == Bound::UPPER and tt_score <= alpha)))
//						return tt_score;
//				}
//			}
//		}

		position_counter++;

//		if (not actions.isRoot() and not actions.performed_pattern_update)
//		{
//			pattern_calculator.addMove(actions.last_move);
//			actions.performed_pattern_update = true;
//		}
//		std::cout << "last tutaj\n";

		if (actions.isEmpty())
		{
//			std::cout << "generating\n";
			actions.clear();
			const Score static_score = move_generator.generate(actions, movegen_mode);
			action_stack.increment(actions.size() + 1);

			if (static_score.isProven())
				return static_score;
		}

//		inference_nnue.update(pattern_calculator);
		if (depthRemaining <= 0)
			return evaluate();

		const Score original_alpha = alpha;
		Score best_score = Score::minus_infinity();
		int moves_checked = 0;
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

//			if (depthRemaining == 2)
//				std::cout << "\nchecking action " << i << "/" << actions.size() << " : " << actions[i].move.text() << " with alpha=" << alpha
//						<< ", beta=" << beta << '\n';

			if (actions[i].score.isUnproven() and position_counter < max_positions)
			{
				// construct next ply action list
				ActionList next_ply_actions = action_stack.create_from_actions(actions, i);
				const Move move = actions[i].move;

				shared_table.getHashFunction().updateHash(hash_key, move);
				shared_table.prefetch(hash_key);

				if (pattern_calculator.signAt(move.row, move.col) != Sign::NONE)
				{
					pattern_calculator.print();
					pattern_calculator.printAllThreats();
					actions.print();
				}

				pattern_calculator.addMove(move);
				inference_nnue.update(pattern_calculator);

				actions[i].score = invert_up(recursive_solve(depthRemaining - 1, invert_down(beta), invert_down(alpha), next_ply_actions));
//				actions[i].size = next_ply_actions.size();

//				if (i == 0)
//					actions[i].score = invert_up(recursive_solve(depthRemaining - 1, invert_down(beta), invert_down(alpha), next_ply_actions));
//				else
//				{
//					actions[i].score = invert_up(recursive_solve(depthRemaining - 1, invert_down(alpha) - 1, invert_down(alpha), next_ply_actions));
//					if (actions[i].score > alpha)
//						actions[i].score = invert_up(recursive_solve(depthRemaining - 1, invert_down(beta), invert_down(alpha), next_ply_actions));
//				}

//				if (next_ply_actions.performed_pattern_update)
				{
					pattern_calculator.undoMove(move);
					inference_nnue.update(pattern_calculator);
				}
				shared_table.getHashFunction().updateHash(hash_key, move);
			}

//			const std::string tab = (depthRemaining == 1) ? "\t" : "";
//
//			std::cout << tab << depthRemaining << " : action " << i << "/" << actions.size() << " : " << actions[i].move.text() << " : Q="
//					<< actions[i].score << "(" << toString(actions[i].bound) << "), best=" << best_score << " : alpha=" << alpha << ", beta=" << beta
//					<< "\n";

			if (actions[i].score > best_score)
			{
				best_score = actions[i].score;
				best_move = actions[i].move;
				if (not actions.isRoot())
				{ // we want to have exact scores at root
					if (best_score >= beta) // or best_score.isWin())
					{
//						std::cout << tab << "beta cutoff : " << best_score << " >= " << beta << "\n";
						break;// fail-soft beta-cutoff
					}
					if (actions[i].score > alpha)
					{
//						std::cout << tab << "raising alpha : " << alpha << " -> " << actions[i].score << "\n";
						alpha = actions[i].score;
					}
				}
			}
			moves_checked++;
		}
//		if (best_score.isLoss() and (moves_checked < actions.size() or not actions.is_fully_expanded))
//			best_score = evaluate();
//
		{ // insert new data to the hash table
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
		}

		assert(best_score != Score::minus_infinity());
		return best_score;
	}
	bool AlphaBetaSearch::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

	Score AlphaBetaSearch::evaluate()
	{
//		return Score();

		return Score(static_cast<int>(2000 * inference_nnue.forward() - 1000));

//		const Sign own_sign = pattern_calculator.getSignToMove();
//		const Sign opponent_sign = invertSign(own_sign);
//		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
//		const int best_threat = static_cast<int>(ThreatType::FIVE);
//
//		static const std::array<int, 10> own_values = { 0, 0, 19, 49, 76, 170, 33, 159, 252, 0 };
//		static const std::array<int, 10> opp_values = { 0, 0, -1, -50, -45, -135, -14, -154, -496, 0 };
//		int result = 12;
//		for (int i = worst_threat; i <= best_threat; i++)
//		{
//			result += own_values[i] * pattern_calculator.getThreatHistogram(own_sign).numberOf(static_cast<ThreatType>(i));
//			result += opp_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).numberOf(static_cast<ThreatType>(i));
//		}
//		return Score(std::max(-1000, std::min(1000, result)));
	}

} /* namespace ag */

