/*
 * MinimaxSearch.cpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/MinimaxSearch.hpp>
#include <alphagomoku/ab_search/StaticSolver.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <cassert>

namespace
{
	using namespace ag;

	ProvenValue convert(Score score) noexcept
	{
		switch (score.getProvenScore())
		{
			case ProvenScore::LOSS:
				return ProvenValue::LOSS;
			case ProvenScore::DRAW:
				return ProvenValue::DRAW;
			default:
			case ProvenScore::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case ProvenScore::WIN:
				return ProvenValue::WIN;
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
			action_stack(get_max_nodes(gameConfig))
	{
	}
	std::pair<Move, Score> MinimaxSearch::solve(SearchTask &task, int depth)
	{
		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());

//		std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//		pattern_calculator.print();
//		pattern_calculator.printAllThreats();

		ActionList actions = action_stack.getRoot();
		Score score;
//		std::cout << "MinimaxSearch\n";
		for (int i = depth; i <= depth; i++)
		{
//			double t0 = getTime();
			score = recursive_solve(i, actions);
//			std::cout << "depth " << i << " : " << score << " : " << (getTime() - t0) << "s\n";
			if (score.isProven())
				break;
		}

//		actions.print(); // TODO remove this later

		for (auto iter = actions.begin(); iter < actions.end(); iter++)
//			if (iter->score.isProven())
			task.addPriorEdge(iter->move, Value(), convert(iter->score));

		switch (actions.getScore().getProvenScore())
		{
			default:
				break;
			case ProvenScore::LOSS:
				task.setValue(Value(0.0f, 0.0f, 1.0f));
				task.setProvenValue(ProvenValue::LOSS);
				task.markAsReadySolver();
				break;
			case ProvenScore::DRAW:
				task.setValue(Value(0.0f, 1.0f, 0.0f));
				task.setProvenValue(ProvenValue::DRAW);
				task.markAsReadySolver();
				break;
			case ProvenScore::WIN:
				task.setValue(Value(1.0f, 0.0f, 0.0f));
				task.setProvenValue(ProvenValue::WIN);
				task.markAsReadySolver();
				break;
		}
		assert(actions.size() > 0);
		return std::pair<Move, Score>(std::max_element(actions.begin(), actions.end())->move, score);
	}
	/*
	 * private
	 */
	Score MinimaxSearch::recursive_solve(int depthRemaining, ActionList &actions)
	{
		assert(depthRemaining > 0);
		MoveGenerator move_generator(game_config, pattern_calculator, actions, MoveGeneratorMode::REDUCED);
		move_generator.generateAll();

		Score score = Score::min_value();
		const bool is_next_move_a_draw = pattern_calculator.getCurrentDepth() + 1 >= game_config.rows * game_config.cols;
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move move = iter->move;
			if (pattern_calculator.getThreatAt(move.sign, move.row, move.col) == ThreatType::FIVE and not is_next_move_a_draw)
				iter->score = Score::win_in(1);
			else
			{
				if (pattern_calculator.isForbidden(move.sign, move.row, move.col))
					iter->score = Score::loss_in(1);
				else
				{
					if (is_next_move_a_draw)
						iter->score = Score::draw();
					else
					{
						if (depthRemaining > 1)
						{
							ActionList next_ply_actions(actions);
							pattern_calculator.addMove(move);
							iter->score = -recursive_solve(depthRemaining - 1, next_ply_actions);
							iter->score.increaseDistanceToWinOrLoss();
							pattern_calculator.undoMove(move);
						}
						else
							iter->score = -evaluate();
					}
				}
			}
			score = std::max(score, iter->score);
			if (score.isWin())
				break;
		}

		return score;
	}
	Score MinimaxSearch::evaluate()
	{
		const Sign own_sign = pattern_calculator.getSignToMove();
		const Sign opponent_sign = invertSign(own_sign);
		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
		const int best_threat = static_cast<int>(ThreatType::FIVE);

		static const std::array<int, 10> own_threat_values = { 0, 0, 1, 5, 10, 50, 100, 100, 1000, 0 };
		static const std::array<int, 10> opp_threat_values = { 0, 0, 0, 0, 1, 5, 10, 10, 100, 0 };
		int result = 0;
		for (int i = worst_threat; i <= best_threat; i++)
		{
			result += own_threat_values[i] * pattern_calculator.getThreatHistogram(own_sign).get(static_cast<ThreatType>(i)).size();
			result -= opp_threat_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).get(static_cast<ThreatType>(i)).size();
		}
		return std::max(-1000, std::min(1000, result));
	}
} /* namespace ag */

