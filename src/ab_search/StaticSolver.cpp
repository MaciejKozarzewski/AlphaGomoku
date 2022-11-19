/*
 * StaticSolver.cpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/StaticSolver.hpp>
#include <alphagomoku/ab_search/Score.hpp>
#include <alphagomoku/ab_search/ActionList.hpp>

#include <cassert>

namespace ag
{
	StaticSolver::StaticSolver(GameConfig gameConfig, PatternCalculator &calc) :
			CommonBase(gameConfig, calc),
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols)
	{
	}
	void StaticSolver::setDrawAfter(int moves) noexcept
	{
		number_of_moves_for_draw = moves;
	}
	Score StaticSolver::solve(ActionList &actions, int depth)
	{
		actions.clear();
		SolverResult result = check_win_in_1(actions);
		if (result.can_continue)
			result = check_draw_in_1(actions);
		if (result.can_continue and depth >= 2)
			result = check_loss_in_2(actions);
		if (result.can_continue and depth >= 3)
			result = check_win_in_3(actions);
		if (result.can_continue and depth >= 5)
			result = check_win_in_5(actions);
		return result.score;
	}
	/*
	 * private
	 */
	StaticSolver::SolverResult StaticSolver::check_win_in_1(ActionList &actions)
	{
		const std::vector<Location> &own_fives = get_own_threats(ThreatType::FIVE);
		if (own_fives.size() > 0)
		{
			for (auto iter = own_fives.begin(); iter < own_fives.end(); iter++)
				actions.add(Move(get_own_sign(), *iter), Score::win_in(1));
			return SolverResult(false, Score::win_in(1));
		}
		return SolverResult();
	}
	StaticSolver::SolverResult StaticSolver::check_draw_in_1(ActionList &actions)
	{
		if (pattern_calculator.getCurrentDepth() + 1 >= number_of_moves_for_draw) // detect draw
		{
			if (is_anything_forbidden_for(get_own_sign()))
			{ // only if there can be any forbidden move, so in renju and for cross (black) player
				std::vector<Location> &temp = get_temporary_list();
				// loop over all moves that can lead to draw, excluding those that are obviously forbidden (overlines and 4x4 forks)
				// 3x3 forks are collected in a temporary list to be checked later
				for (int row = 0; row < game_config.rows; row++)
					for (int col = 0; col < game_config.cols; col++)
						if (pattern_calculator.signAt(row, col) == Sign::NONE)
						{
							const ThreatType threat = pattern_calculator.getThreatAt(get_own_sign(), row, col);
							switch (threat)
							{
								default: // no forbidden threat, add this move and exit
									actions.add(Move(row, col, get_own_sign()), Score::draw());
									return SolverResult(false, Score::draw());
								case ThreatType::FORK_3x3: // possibly forbidden, may require costly checking
									temp.push_back(Location(row, col));
									break;
								case ThreatType::FORK_4x4: // obviously forbidden
								case ThreatType::OVERLINE: // obviously forbidden
									actions.add(Move(row, col, get_own_sign()), Score::loss_in(0));
									break;
							}
						}

				// now check 3x3 forks as this may require add/undo move
				for (auto move = temp.begin(); move < temp.end(); move++)
				{
					if (is_own_3x3_fork_forbidden(*move))
						actions.add(Move(*move, get_own_sign()), Score::loss_in(0));
					else
					{ // found non-forbidden 3x3 fork, can play that to draw
						actions.add(Move(*move, get_own_sign()), Score::draw());
						return SolverResult(false, Score::draw());
					}
				}
			}
			else
			{ // if there are no forbidden moves, just find the first empty spot on board
				for (int row = 0; row < game_config.rows; row++)
					for (int col = 0; col < game_config.cols; col++)
						if (pattern_calculator.signAt(row, col) == Sign::NONE)
						{ // found at least one move that leads to draw, can exit now
							actions.add(Move(row, col, get_own_sign()), Score::draw());
							return SolverResult(false, Score::draw());
						}
			}
		}
		return SolverResult();
	}
	StaticSolver::SolverResult StaticSolver::check_loss_in_2(ActionList &actions)
	{
		const std::vector<Location> &opponent_fives = get_copy(get_opponent_threats(ThreatType::FIVE)); // this must be copied as later check for forbidden moves may change the order of elements
		switch (opponent_fives.size())
		{
			case 0: // no fives
				return SolverResult();
			case 1: // single five
			{
				// check if any of the defensive moves is at the same time a win for us (but only if there is at most single five for the opponent)
				const Move five_move = opponent_fives[0];
				if (is_caro_rule())
				{ // in caro there may be multiple defensive moves to the threat of five
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), five_move.row, five_move.col);
					const Direction dir = group.findDirectionOf(PatternType::FIVE);
					const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), five_move.row,
							five_move.col, dir);
					for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
					{
						switch (get_own_threat_at(iter->row, iter->col))
						{
							case ThreatType::OPEN_4:
							case ThreatType::FORK_4x4:
							{
								actions.add(Move(get_own_sign(), *iter), Score::win_in(3));
								return SolverResult(false, Score::win_in(3));
							}
							case ThreatType::FORK_4x3:
							{
								const Score solution = try_solve_fork_4x3(*iter);
								if (solution.isWin())
								{
									actions.add(Move(get_own_sign(), *iter), solution);
									return SolverResult(false, solution);
								}
								break;
							}
							case ThreatType::FORK_3x3:
							{
								if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour())
								{
									actions.add(Move(get_own_sign(), *iter), Score::win_in(5));
									return SolverResult(false, Score::win_in(5));
								}
								break;
							}
							default:
								break;
						}
					}
				}
				else
				{ // for rules other than caro there is exactly one defensive move to the threat of five (the same move that creates a five)
					const Move response(get_own_sign(), five_move);
					switch (get_own_threat_at(response.row, response.col))
					{
						case ThreatType::OPEN_4:
						{
							actions.add(response, Score::win_in(3));
							return SolverResult(false, Score::win_in(3));
						}
						case ThreatType::FORK_4x4:
						{
							if (is_own_4x4_fork_forbidden())
							{ //relevant only for renju rule
								actions.add(response, Score::loss_in(2)); // the defensive move is forbidden -> loss
								return SolverResult(false, Score::loss_in(2));
							}
							else
							{
								actions.is_threatening = true;
								actions.add(response, Score::win_in(3));
								return SolverResult(false, Score::win_in(3));
							}
						}
						case ThreatType::FORK_4x3:
						{
							const Score solution = try_solve_fork_4x3(five_move);
							if (solution.isWin())
							{
								actions.add(response, solution);
								return SolverResult(false, solution);
							}
							break;
						}
						case ThreatType::FORK_3x3:
						{
							if (is_own_3x3_fork_forbidden(response.location()))
							{ //relevant only for renju rule
								actions.add(response, Score::loss_in(2)); // the defensive move is forbidden -> loss
								return SolverResult(false, Score::loss_in(2));
							}
							else
							{
								if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour())
								{
									actions.is_threatening = true;
									actions.add(response, Score::win_in(5));
									return SolverResult(false, Score::win_in(5));
								}
							}
							break;
						}
						default:
							break;
					}
				}
				return SolverResult(false, Score()); // although there is no proven score, we don't have to continue checking another threats
			}
			default: // multiple fives
			{
				if (is_caro_rule())
				{ // in caro rule it is possible to block two threats of five with a single move, for example "!_XXXX_O"
					Move found_move(-1, -1);
					bool flag = true;
					for (auto five_move = opponent_fives.begin(); five_move < opponent_fives.end() and flag; five_move++)
					{ // loop over all opponent fives to find defensive moves
						const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), five_move->row,
								five_move->col);
						const Direction dir = group.findDirectionOf(PatternType::FIVE);
						const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), five_move->row,
								five_move->col, dir);
						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
						{ // loop over all defensive moves to check if they are all at the same spot
							if (found_move.row == -1 and found_move.col == -1)
								found_move = *iter; // found first move
							if (found_move != *iter)
							{ // found second required defensive move that is somewhere else -> it is a loss (we can make only one move)
								flag = false;
								break;
							}
						}
					}
					if (found_move.row != -1 and found_move.col != -1)
						return SolverResult(false, Score()); // we have found single defensive move to refute all fives
				}
				// if we got here it means that either:
				//  - it is caro rule and there is no single move that would defend against all opponent fives, or
				//  - it is not caro rule so multiple opponent fives are always losing
				for (auto iter = opponent_fives.begin(); iter < opponent_fives.end(); iter++)
					actions.add(Move(get_own_sign(), *iter), Score::loss_in(2)); // just add some moves marking them as loss
				return SolverResult(false, Score::loss_in(2));
			}
		}
	}
	StaticSolver::SolverResult StaticSolver::check_win_in_3(ActionList &actions)
	{
		const std::vector<Location> &own_open_four = get_own_threats(ThreatType::OPEN_4);
		const std::vector<Location> &own_fork_4x4 = get_own_threats(ThreatType::FORK_4x4);
		const int own_win_in_3_count = own_open_four.size() + (is_own_4x4_fork_forbidden() ? 0 : own_fork_4x4.size());
		if (own_win_in_3_count > 0)
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				actions.add(Move(get_own_sign(), *iter), Score::win_in(3));
			if (not is_own_4x4_fork_forbidden())
				for (auto iter = own_fork_4x4.begin(); iter < own_fork_4x4.end(); iter++)
					actions.add(Move(get_own_sign(), *iter), Score::win_in(3));
			return SolverResult(false, Score::win_in(3));
		}
		return SolverResult();
	}
	StaticSolver::SolverResult StaticSolver::check_win_in_5(ActionList &actions)
	{
		const std::vector<Location> &own_fork_4x3 = get_own_threats(ThreatType::FORK_4x3);
		for (auto iter = own_fork_4x3.begin(); iter < own_fork_4x3.end(); iter++)
		{
			const Score solution = try_solve_fork_4x3(*iter);
			if (solution.isWin())
			{
				actions.add(Move(get_own_sign(), *iter), solution);
				return SolverResult(false, solution);
			}
		}

		if (not is_anything_forbidden_for(get_own_sign()))
		{ // in renju, for cross (black) player it is possible that in 3x3 fork some open 3 can't be converted to a four because they will be forbidden for some reason, so we check only circle (white player here)
			if (pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour() == false)
			{
				const std::vector<Location> &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
				for (auto iter = own_fork_3x3.begin(); iter < own_fork_3x3.end(); iter++)
				{
					actions.add(Move(get_own_sign(), *iter), Score::win_in(5));
					return SolverResult(false, Score::win_in(5));
				}
			}
		}
		return SolverResult();
	}
	Score StaticSolver::try_solve_fork_4x3(Move move)
	{
		assert(move.sign == Sign::NONE);

		if (is_anything_forbidden_for(get_own_sign()))
			return Score(); // there is a possibility that inside the three there will later be a forbidden move -> too complicated to check statically

		const DirectionGroup<PatternType> own_patterns = pattern_calculator.getPatternTypeAt(get_own_sign(), move.row, move.col);
		const Direction direction_of_own_four = own_patterns.findDirectionOf(PatternType::HALF_OPEN_4);
		ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move.row, move.col,
				direction_of_own_four);
		defensive_moves.remove(move.location());

		ThreatType best_opponent_threat = ThreatType::NONE;
		for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
			best_opponent_threat = std::max(best_opponent_threat, get_opponent_threat_at(iter->row, iter->col));

		switch (best_opponent_threat)
		{
			default:
			case ThreatType::NONE:
			case ThreatType::HALF_OPEN_3:
			case ThreatType::OPEN_3:
				return Score::win_in(5); // opponent cannot make any four in response to our threat
			case ThreatType::FORK_3x3:
			{
//				return Score::win_in(5); // TODO maybe it is not worth checking for opponent forbidden moves, it is a win anyway, doesn't matter if longer or shorter
				if (is_anything_forbidden_for(get_opponent_sign()))
				{
					assert(defensive_moves.size() == 1);
					const Location tmp = defensive_moves[0];
					return is_opponent_3x3_fork_forbidden(tmp) ? Score::win_in(3) : Score::win_in(5);
				}
				else
					return Score::win_in(5);
			}
			case ThreatType::HALF_OPEN_4:
			case ThreatType::FORK_4x3:
				return Score(); // too complicated to check statically
			case ThreatType::FORK_4x4:
				return is_anything_forbidden_for(get_opponent_sign()) ? Score::win_in(3) : Score::loss_in(4);
			case ThreatType::OPEN_4:
				return Score::loss_in(4); // opponent can make either open four
			case ThreatType::FIVE:
				return Score::loss_in(2); // opponent can make a five
			case ThreatType::OVERLINE:
				return is_anything_forbidden_for(get_opponent_sign()) ? Score::win_in(3) : Score::loss_in(2);
		}
	}

} /* namespace ag */

