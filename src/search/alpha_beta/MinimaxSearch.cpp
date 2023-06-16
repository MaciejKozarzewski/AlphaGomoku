/*
 * MinimaxSearch.cpp
 *
 *  Created on: Jun 1, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/MinimaxSearch.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/search/Score.hpp>

namespace
{
	using namespace ag;

	Value convertScoreToValue(Score score) noexcept
	{
		switch (score.getProvenValue())
		{
			case ProvenValue::LOSS:
				return Value::loss();
			case ProvenValue::DRAW:
				return Value::draw();
			default:
			case ProvenValue::UNKNOWN:
				return score.convertToValue();
			case ProvenValue::WIN:
				return Value::win();
		}
	}
	int get_max_nodes(GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
	}
}

namespace ag
{
	MinimaxSearch::MinimaxSearch(GameConfig gameConfig) :
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			move_generator(gameConfig, pattern_calculator),
			action_stack(get_max_nodes(gameConfig))
	{
	}
	void MinimaxSearch::solve(SearchTask &task, int depth, MoveGeneratorMode gen_mode)
	{
		generator_mode = gen_mode;
		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());

		ActionList actions = action_stack.create_root();

//		double t0 = getTime();
		Score result = recursive_solve(depth, actions);
//		std::cout << "depth " << depth << " : " << result << " : " << (getTime() - t0) << "s\n";

//		std::sort(actions.begin(), actions.end());
//		actions.print(); // TODO remove this later

		task.getActionScores().fill(Score::min_value());
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			task.getActionScores().at(m.row, m.col) = iter->score;
			if (actions.must_defend)
				task.addDefensiveMove(m);
		}
		task.setScore(result);
		task.markAsProcessedBySolver();
	}
	/*
	 * private
	 */
	Score MinimaxSearch::recursive_solve(int depthRemaining, ActionList &actions)
	{
		assert(depthRemaining >= 0);

		actions.clear();
		Score static_score = move_generator.generate(actions, generator_mode);
		action_stack.increment(actions.size() + 1);
		if (static_score.isProven())
		{
			if (not actions.isRoot())
				action_stack.decrement(actions.size() + 1);
			return static_score;
		}

		if (depthRemaining == 0)
		{
			if (not actions.isRoot())
				action_stack.decrement(actions.size() + 1);
			return evaluate();
		}

		Score score = Score::min_value();
		for (int i = 0; i < actions.size(); i++)
		{
			ActionList next_ply_actions = action_stack.create_from_actions(actions, i);
			const Move move = actions[i].move;
			pattern_calculator.addMove(move);
			actions[i].score = invert_up(recursive_solve(depthRemaining - 1, next_ply_actions));
			pattern_calculator.undoMove(move);

			score = std::max(score, actions[i].score);
			if (score.isWin())
				break;
		}
		if (not actions.isRoot())
			action_stack.decrement(actions.size() + 1);
		return score;

//		if (actions.isEmpty())
//		{
//			Score static_score = move_generator.generate(actions, generator_mode);
//			action_stack.increment(actions.size() + 1);
//			if (static_score.isProven())
//				return static_score;
//		}
//
//		if (depthRemaining == 0)
//			return evaluate();
//
//		Score score = Score::min_value();
//		for (int i = 0; i < actions.size(); i++)
//		{
//			if (nodes_left > 0 and not actions[i].score.isProven())
//			{
//				nodes_left--;
//
//				ActionList next_ply_actions = action_stack.create_from_action(actions[i]);
//				const Move move = actions[i].move;
//				pattern_calculator.addMove(move);
//				actions[i].score = -recursive_solve(depthRemaining - 1, next_ply_actions);
//				actions[i].score.increaseDistance();
//				pattern_calculator.undoMove(move);
//			}
//
//			score = std::max(score, actions[i].score);
////			if (score.isWin())
////				break;
//		}
//		return score;
	}
	Score MinimaxSearch::evaluate()
	{
		return Score(randInt(-10, 11) + randInt(-10, 11) + randInt(-10, 11));

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
//		return std::max(-1000, std::min(1000, result));
	}
} /* namespace ag */

