/*
 * ThreatGenerator.cpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/tss/ThreatGenerator.hpp>
#include <alphagomoku/tss/Score.hpp>
#include <alphagomoku/tss/ActionList.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <cassert>
#include <x86intrin.h>

namespace
{
	using namespace ag;

	bool contains(const std::vector<Location> &list, Location m) noexcept
	{
		return std::find(list.begin(), list.end(), m) != list.end();
	}

	/*
	 * \brief Creates the list of elements that occur in both arguments.
	 */
	template<int N, int M>
	void get_intersection(ShortVector<Location, N> &lhs, const ShortVector<Location, M> &rhs) noexcept
	{
		int i = 0;
		while (i < lhs.size())
		{
			if (rhs.contains(lhs[i]))
				i++;
			else
				lhs.remove(i);
		}
	}
	/*
	 * \brief Creates the list of elements that occur in both arguments.
	 */
	template<int N, int M>
	int get_common_moves_count(const ShortVector<Location, N> &lhs, const ShortVector<Location, M> &rhs) noexcept
	{
		int result = 0;
		for (int i = 0; i < rhs.size(); i++)
			if (lhs.contains(rhs[i]))
				result++;
		return result;
	}
	/*
	 * \brief Creates the list of elements that occur in both arguments.
	 */
	template<int N, int M>
	void get_union(ShortVector<Location, N> &lhs, const ShortVector<Location, M> &rhs) noexcept
	{
		for (int i = 0; i < rhs.size(); i++)
			if (not lhs.contains(rhs[i]))
				lhs.add(rhs[i]);
	}

	bool is_a_three(PatternType pt) noexcept
	{
		return pt == PatternType::HALF_OPEN_3 or pt == PatternType::OPEN_3;
	}
	bool is_a_four(PatternType pt) noexcept
	{
		return pt == PatternType::HALF_OPEN_4 or pt == PatternType::OPEN_4 or pt == PatternType::DOUBLE_4;
	}

	struct DefensiveMoves
	{
		private:
			bool not_initialized = true;
			ShortVector<Location, 24> list;
		public:
			template<int N>
			void get_intersection_with(const ShortVector<Location, N> &other)
			{
				if (not_initialized)
				{
					list.add(other);
					not_initialized = false;
				}
				else
					get_intersection(list, other);
			}
			bool is_empty() const noexcept
			{
				return list.size() == 0;
			}
			const ShortVector<Location, 24>& get_list() const noexcept
			{
				return list;
			}
	};
}

namespace ag
{
	namespace tss
	{
		ThreatGenerator::ThreatGenerator(GameConfig gameConfig, PatternCalculator &calc) :
				game_config(gameConfig),
				pattern_calculator(calc),
				moves(gameConfig.rows, gameConfig.cols),
				number_of_moves_for_draw(gameConfig.rows * gameConfig.cols)
		{
			temporary_list.reserve(64);
		}
		Score ThreatGenerator::generate(ActionList &actions, GeneratorMode mode)
		{
			assert(this->actions == nullptr);
			this->actions = &actions;
			moves.fill(false);

			actions.clear();
			Result result = try_win_in_1();
			if (result.can_continue)
				result = try_draw_in_1();
			if (mode >= GeneratorMode::STATIC)
			{
				if (result.can_continue)
					result = defend_loss_in_2();
				if (result.can_continue)
					result = try_win_in_3();
				if (result.can_continue)
					result = defend_loss_in_4();
				if (result.can_continue)
					result = try_win_in_5();
			}
			if (result.can_continue and mode >= GeneratorMode::VCF)
			{
				const int count = add_own_half_open_fours();
				if (count > 0)
					actions.has_initiative = true;
//				if (mode >= GeneratorMode::VCT)
//					add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::OPEN_3));
			}
			this->actions = nullptr;
			return result.score;
		}
		/*
		 * private
		 */
		template<ThreatGenerator::AddMode Mode>
		void ThreatGenerator::add_move(Location m, Score s) noexcept
		{
			if (moves.at(m.row, m.col) == true)
			{
				if (Mode == OVERRIDE_DUPLICATE)
				{
					for (auto iter = actions->begin(); iter < actions->end(); iter++)
						if (iter->move.location() == m)
						{
							iter->score = s;
							return;
						}
					assert(false); // the must must be contained in the action list
				}
			}
			else
			{
				actions->add(Move(get_own_sign(), m.row, m.col), s);
				moves.at(m.row, m.col) = true;
			}
		}
		template<ThreatGenerator::AddMode Mode>
		void ThreatGenerator::add_moves(const std::vector<Location> &moves, Score s) noexcept
		{
			for (auto iter = moves.begin(); iter < moves.end(); iter++)
				add_move<Mode>(*iter, s);
		}
		template<ThreatGenerator::AddMode Mode, typename T>
		void ThreatGenerator::add_moves(const T begin, const T end, Score s) noexcept
		{
			for (auto iter = begin; iter < end; iter++)
				add_move<Mode>(*iter, s);
		}
		ShortVector<Location, 6> ThreatGenerator::get_defensive_moves(Location move, Direction dir)
		{
			ShortVector<Location, 6> result = pattern_calculator.getDefensiveMoves(get_own_sign(), move.row, move.col, dir);
			if (is_anything_forbidden_for(get_own_sign()))
			{
				int i = 0;
				while (i < result.size())
				{
					if (pattern_calculator.isForbidden(get_own_sign(), result[i].row, result[i].col))
						result.remove(i);
					else
						i++;
				}
			}
			else
			{
				if (is_anything_forbidden_for(get_opponent_sign()))
				{ // in renju there may be an additional defensive move against cross (black) open four that is blocked by a forbidden move on one end
					const PatternType pt = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move.row, move.col, dir);
					if (pt == PatternType::OPEN_4)
					{
						const uint32_t raw = pattern_calculator.getExtendedPatternAt(move.row, move.col, dir);
						const int dist = ((raw & 65520u) == 1344u) ? -1 : (((raw & 4193280u) == 344064u) ? +1 : 0); // detect patterns '_XXX!_' or  '_!XXX_'
						if (dist != 0)
						{ // the type of open four is the 'straight four' one
							const Location loc = shiftInDirection(dir, 4 * dist, move); // get position of the other side of the open four
							if (pattern_calculator.isForbidden(get_opponent_sign(), loc.row, loc.col))
							{ // we have a pattern like '?XXX!!' where '?' is forbidden so there are two defensive moves at '!'
								const Location loc2 = shiftInDirection(dir, -1 * dist, move);
								result.add(loc2);
							}
						}
					}
				}
			}
			return result;
		}
		ThreatGenerator::Result ThreatGenerator::try_win_in_1()
		{
			assert(actions->size() == 0);
			assert(actions->must_defend == false && actions->has_initiative == false);

			const std::vector<Location> &own_fives = get_own_threats(ThreatType::FIVE);
			if (own_fives.size() > 0)
			{
				actions->has_initiative = true;
				add_moves<EXCLUDE_DUPLICATE>(own_fives, Score::win_in(1));
				return Result(false, Score::win_in(1));
			}
			return Result();
		}
		ThreatGenerator::Result ThreatGenerator::try_draw_in_1()
		{
			assert(actions->size() == 0);
			assert(actions->must_defend == false && actions->has_initiative == false);

			if (pattern_calculator.getCurrentDepth() + 1 >= number_of_moves_for_draw) // detect draw
			{
				if (is_anything_forbidden_for(get_own_sign()))
				{ // only if there can be any forbidden move, so in renju and for cross (black) player
					temporary_list.clear();
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
										add_move<EXCLUDE_DUPLICATE>(Location(row, col), Score::draw());
										return Result(false, Score::draw());
									case ThreatType::FORK_3x3: // possibly forbidden, may require costly checking
										temporary_list.push_back(Location(row, col));
										break;
									case ThreatType::FORK_4x4: // obviously forbidden
									case ThreatType::OVERLINE: // obviously forbidden
										add_move<EXCLUDE_DUPLICATE>(Location(row, col), Score::loss_in(1));
										break;
								}
							}

					// now check 3x3 forks as this may require costly add/undo move
					for (auto move = temporary_list.begin(); move < temporary_list.end(); move++)
					{
						if (is_own_3x3_fork_forbidden(*move))
							add_move<EXCLUDE_DUPLICATE>(*move, Score::loss_in(1));
						else
						{ // found non-forbidden 3x3 fork, can play it to draw
							add_move<EXCLUDE_DUPLICATE>(*move, Score::draw());
							return Result(false, Score::draw());
						}
					}
					return Result(false, Score::loss_in(1)); // could not find any non-forbidden move
				}
				else
				{ // if there are no forbidden moves, just find the first empty spot on board
					for (int row = 0; row < game_config.rows; row++)
						for (int col = 0; col < game_config.cols; col++)
							if (pattern_calculator.signAt(row, col) == Sign::NONE)
							{ // found at least one move that leads to draw, can stop now
								add_move<EXCLUDE_DUPLICATE>(Location(row, col), Score::draw());
								break;
							}
					return Result(false, Score::draw()); // the game ends up with a draw, it doesn't matter if there is any move to play or not
				}
			}
			return Result();
		}
		ThreatGenerator::Result ThreatGenerator::defend_loss_in_2()
		{
			assert(actions->size() == 0);
			assert(actions->must_defend == false && actions->has_initiative == false);

			const std::vector<Location> &opponent_fives = get_copy(get_opponent_threats(ThreatType::FIVE)); // this must be copied as later check for forbidden moves may change the order of elements
			if (opponent_fives.size() == 0)
				return Result();
			else
				actions->must_defend = true;

			DefensiveMoves defensive_moves;
			for (auto move = opponent_fives.begin(); move < opponent_fives.end(); move++)
			{
				const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
				const Direction direction = patterns.findDirectionOf(PatternType::FIVE);
				const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);

				defensive_moves.get_intersection_with(tmp);
				if (defensive_moves.is_empty())
				{
					add_moves<EXCLUDE_DUPLICATE>(opponent_fives); // we must always produce some moves, even if the threat is not refutable
					return Result(false, Score::loss_in(2));
				}
			}
			Score best_score = Score::min_value();
			for (auto iter = defensive_moves.get_list().begin(); iter < defensive_moves.get_list().end(); iter++)
			{
				Score response_score;
				switch (get_own_threat_at(iter->row, iter->col))
				{
					case ThreatType::FORK_3x3:
					{
						assert(is_own_3x3_fork_forbidden(*iter) == false); // forbidden move should have been excluded earlier
						if (is_anything_forbidden_for(get_own_sign()))
						{ // relevant only for renju rule
							const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_own_sign(), iter->row, iter->col);
							if (patterns.contains(PatternType::OPEN_4))
							{ // there is an open four hidden inside a legal 3x3 fork
								actions->has_initiative = true;
								response_score = Score::win_in(3);
							}
						}
						else
						{ // for rules other than renju
							if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour())
							{ // we have 3x3 fork while the opponent has no four
								actions->has_initiative = true;
								response_score = Score::win_in(5);
							}
						}
						break;
					}
					case ThreatType::FORK_4x3:
					{
						const Score solution = try_solve_own_fork_4x3(*iter);
						if (solution.isWin())
						{ // found a surely winning 4x3 fork
							actions->has_initiative = true;
							response_score = solution;
						}
						break;
					}
					case ThreatType::FORK_4x4:
					{
						assert(is_own_4x4_fork_forbidden() == false); // forbidden move should have been excluded earlier
						actions->has_initiative = true;
						response_score = Score::win_in(3);
						break;
					}
					case ThreatType::OPEN_4:
					{
						actions->has_initiative = true;
						response_score = Score::win_in(3);
						break;
					}
					default:
						break;
				}
				const DirectionGroup<PatternType> tmp = pattern_calculator.getPatternTypeAt(get_own_sign(), iter->row, iter->col);
				if (tmp.contains(PatternType::HALF_OPEN_4))
					actions->has_initiative = true;  // there is some half-open four to make in a response (may help gain initiative)

				add_move<EXCLUDE_DUPLICATE>(*iter, response_score);
				best_score = std::max(best_score, response_score);
			}
			return Result(false, best_score); // although there is no proven score, we don't have to continue checking another threats
		}
		ThreatGenerator::Result ThreatGenerator::try_win_in_3()
		{
			assert(actions->size() == 0);
			assert(actions->must_defend == false && actions->has_initiative == false);
			int hidden_open_four_count = 0;
			if (is_anything_forbidden_for(get_own_sign()))
			{ // in renju there might be an open four hidden inside a legal 3x3 fork
				const std::vector<Location> &opp_fork_3x3 = get_copy(get_own_threats(ThreatType::FORK_3x3));
				for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
				{
					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
					if (patterns.contains(PatternType::OPEN_4) and not is_own_3x3_fork_forbidden(*move))
					{
						hidden_open_four_count++;
						add_move<EXCLUDE_DUPLICATE>(*move, Score::win_in(3));
					}
				}
			}

			const std::vector<Location> &own_open_four = get_own_threats(ThreatType::OPEN_4);
			add_moves<EXCLUDE_DUPLICATE>(own_open_four, Score::win_in(3));
			if (hidden_open_four_count + own_open_four.size() > 0)
			{
				actions->has_initiative = true;
				return Result(false, Score::win_in(3));
			}

			const std::vector<Location> &own_fork_4x4 = get_own_threats(ThreatType::FORK_4x4);
			if (own_fork_4x4.size() > 0 and not is_own_4x4_fork_forbidden())
			{
				actions->has_initiative = true;
				add_moves<EXCLUDE_DUPLICATE>(own_fork_4x4, Score::win_in(3));
				return Result(false, Score::win_in(3));
			}
			return Result();
		}
		ThreatGenerator::Result ThreatGenerator::defend_loss_in_4()
		{
			assert(actions->size() == 0);
			assert(actions->must_defend == false && actions->has_initiative == false);

			if (pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour())
			{ // any four that we can make may help us to gain initiative
				actions->has_initiative = true;
				const Score best_score = add_own_4x3_forks();
				if (best_score.isWin())
					return Result(false, best_score);
				else
					add_own_half_open_fours(); // it makes sense to add other threats only if there is no winning 4x3 fork
			}

			if (game_config.rules != GameRules::RENJU)
			{
				// in order to find a minimal number of defensive moves we must calculate an intersection of defensive moves to all individual threats
				// if there are any moves that occur as a refutation to all threats we can make them
				// otherwise, if there is no single move that can defend against all threats it is a loss in 4 (provided that we can't make any own fours)
				DefensiveMoves defensive_moves;

				const std::vector<Location> &opp_open_four = get_opponent_threats(ThreatType::OPEN_4);
				for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
				{
					actions->must_defend = true;
					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
					const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);

					defensive_moves.get_intersection_with(tmp);
					if (defensive_moves.is_empty() and not actions->has_initiative)
					{
						add_moves<EXCLUDE_DUPLICATE>(opp_open_four); // we must always produce some moves, even if the threat is not refutable
						return Result(false, Score::loss_in(4));
					}
				}

				const std::vector<Location> &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
				ShortVector<Location, 24> storage;
				for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
				{
					actions->must_defend = true;
					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);

					// at first resolve open and double fours, as they all must be refuted by a single move
					for (Direction dir = 0; dir < 4; dir++)
						if (patterns[dir] == PatternType::OPEN_4 or patterns[dir] == PatternType::DOUBLE_4)
							defensive_moves.get_intersection_with(get_defensive_moves(*move, dir));

					// then resolve half open fours
					// this case is more complicated as we actually only need to refute all but one of them
					// we would need to check all combinations and pick the one that has the largest intersection with the defensive moves
					// and then test it against all combinations for other forks...
					// because this is too complicated, we approximate the result by taking union of defensive moves to all half open fours
					// it produces more moves than necessary but at least will not overlook any
					// triple and quadruple forks are quite rare anyway (approximately 1 in 2000 positions)
					if (patterns.count(PatternType::HALF_OPEN_4) > 0)
					{
						storage.clear();
						for (Direction dir = 0; dir < 4; dir++)
							if (patterns[dir] == PatternType::HALF_OPEN_4)
								get_union(storage, get_defensive_moves(*move, dir));
						defensive_moves.get_intersection_with(storage);
					}

					if (defensive_moves.is_empty() and not actions->has_initiative)
					{
						add_moves<EXCLUDE_DUPLICATE>(opp_fork_4x4); // we must always produce some moves, even if the threat is not refutable
						return Result(false, Score::loss_in(4));
					}
				}
				add_moves<EXCLUDE_DUPLICATE>(defensive_moves.get_list().begin(), defensive_moves.get_list().end());
			}
			else
			{ // in renju rule there are too complex dependencies between defensive and forbidden moves
				{ // artificial scope so that the list below is not used later
					const std::vector<Location> &opp_open_four = get_copy(get_opponent_threats(ThreatType::OPEN_4));
					for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
					{
						actions->must_defend = true;
						const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
						const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
						const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);
						add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
					}
				}

				if (is_anything_forbidden_for(get_opponent_sign()))
				{ // in renju there might be an open four inside a legal 3x3 fork
					const std::vector<Location> &opp_fork_3x3 = get_copy(get_opponent_threats(ThreatType::FORK_3x3));
					for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
					{
						const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
						if (patterns.contains(PatternType::OPEN_4) and not is_opponent_3x3_fork_forbidden(*move))
						{
							actions->must_defend = true;
							const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
							const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);
							add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
						}
					}
				}

				const std::vector<Location> &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
				if (not is_opponent_4x4_fork_forbidden())
				{
					for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
					{
						actions->must_defend = true;
						const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
						for (Direction dir = 0; dir < 4; dir++)
							if (is_a_four(patterns[dir]))
							{
								const ShortVector<Location, 6> tmp = get_defensive_moves(*move, dir);
								add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
							}
					}
				}
			}
			return Result();
		}
		ThreatGenerator::Result ThreatGenerator::try_win_in_5()
		{
			assert(actions->must_defend == false && actions->has_initiative == false);
			Score best_score = add_own_4x3_forks();

			if (not is_anything_forbidden_for(get_own_sign()))
			{ // in renju, for cross (black) player it is possible that in 3x3 fork some open 3 can't be converted to a four because they will be forbidden for some reason, so we check only circle (white)
				if (pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour() == false)
				{
					const std::vector<Location> &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
					add_moves<EXCLUDE_DUPLICATE>(own_fork_3x3, Score::win_in(5));
					if (own_fork_3x3.size() > 0)
						best_score = std::max(best_score, Score::win_in(5));
				}
			}

			if (best_score.isWin())
			{
				actions->has_initiative = true;
				return Result(false, best_score);
			}
			return Result();
		}
		Score ThreatGenerator::add_own_4x3_forks()
		{
			Score result;
			const std::vector<Location> &own_fork_4x3 = get_own_threats(ThreatType::FORK_4x3);
			for (auto iter = own_fork_4x3.begin(); iter < own_fork_4x3.end(); iter++)
			{
				const Score solution = try_solve_own_fork_4x3(*iter);
				add_move<OVERRIDE_DUPLICATE>(*iter, solution);
				if (solution.isProven())
					result = std::max(result, solution);
			}
			return result;
		}
		int ThreatGenerator::add_own_half_open_fours()
		{
			int hidden_count = 0;
			if (is_anything_forbidden_for(get_own_sign()))
			{ // in renju rule there might be a half-open four hidden inside a legal 3x3 fork
				const std::vector<Location> &own_fork_3x3 = get_copy(get_own_threats(ThreatType::FORK_3x3));
				for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
				{ // loop over 3x3 forks
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
					if (group.contains(PatternType::HALF_OPEN_4) and not is_own_3x3_fork_forbidden(*move))
					{
						for (Direction dir = 0; dir < 4; dir++)
							if (group[dir] == PatternType::HALF_OPEN_4)
							{
								add_move<EXCLUDE_DUPLICATE>(*move);
								hidden_count++;
							}
					}
				}
			}
			add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::HALF_OPEN_4)); // half open four is a threat of win in 1
			return hidden_count + get_own_threats(ThreatType::HALF_OPEN_4).size();
		}
		Score ThreatGenerator::try_solve_own_fork_4x3(Location move)
		{
			if (is_anything_forbidden_for(get_own_sign()))
				return Score(); // there is a possibility that inside the three there will later be a forbidden move -> too complicated to check statically

			const DirectionGroup<PatternType> own_patterns = pattern_calculator.getPatternTypeAt(get_own_sign(), move.row, move.col);
			const Direction direction_of_own_four = own_patterns.findDirectionOf(PatternType::HALF_OPEN_4);
			ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move.row, move.col,
					direction_of_own_four);
			defensive_moves.remove(move);

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
//					return Score::win_in(5); // TODO maybe it is not worth checking for opponent forbidden moves, it is a win anyway, doesn't matter if longer or shorter
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
					return Score::loss_in(4); // opponent can make open four
				case ThreatType::FIVE:
					return Score::loss_in(2); // opponent can make a five
				case ThreatType::OVERLINE:
					return is_anything_forbidden_for(get_opponent_sign()) ? Score::win_in(3) : Score::loss_in(2);
			}
		}
		void ThreatGenerator::check_open_four_in_renju(Location move, Direction direction)
		{
			// sometimes an open four could only be created in one direction, as the move on the other side is forbidden
			const uint32_t raw = pattern_calculator.getExtendedPatternAt(move.row, move.col, direction);
			const int dist = ((raw & 65520u) == 1344u) ? -1 : (((raw & 4193280u) == 344064u) ? +1 : 0); // detect patterns '_XXX!_' or  '_!XXX_'
			if (dist != 0)
			{ // the type of open four is the 'straight four' one
				const Location loc = shiftInDirection(direction, 4 * dist, move);
				// look onto the other side of the open four
				if (pattern_calculator.isForbidden(get_opponent_sign(), loc.row, loc.col))
				{ // we have a pattern like '?XXX!!' where '?' is forbidden so there are two defensive moves at '!'
					const Location loc2 = shiftInDirection(direction, -1 * dist, move);
					add_move<OVERRIDE_DUPLICATE>(loc2);
				}
			}
		}

		bool ThreatGenerator::is_caro_rule() const noexcept
		{
			return game_config.rules == GameRules::CARO5 or game_config.rules == GameRules::CARO6;
		}
		Sign ThreatGenerator::get_own_sign() const noexcept
		{
			return pattern_calculator.getSignToMove();
		}
		Sign ThreatGenerator::get_opponent_sign() const noexcept
		{
			return invertSign(get_own_sign());
		}
		ThreatType ThreatGenerator::get_own_threat_at(int row, int col) const noexcept
		{
			return pattern_calculator.getThreatAt(get_own_sign(), row, col);
		}
		ThreatType ThreatGenerator::get_opponent_threat_at(int row, int col) const noexcept
		{
			return pattern_calculator.getThreatAt(get_opponent_sign(), row, col);
		}
		const std::vector<Location>& ThreatGenerator::get_own_threats(ThreatType tt) const noexcept
		{
			return pattern_calculator.getThreatHistogram(get_own_sign()).get(tt);
		}
		const std::vector<Location>& ThreatGenerator::get_opponent_threats(ThreatType tt) const noexcept
		{
			return pattern_calculator.getThreatHistogram(get_opponent_sign()).get(tt);
		}
		bool ThreatGenerator::is_own_4x4_fork_forbidden() const noexcept
		{
			return is_anything_forbidden_for(get_own_sign());
		}
		bool ThreatGenerator::is_opponent_4x4_fork_forbidden() const noexcept
		{
			return is_anything_forbidden_for(get_opponent_sign());
		}
		bool ThreatGenerator::is_own_3x3_fork_forbidden(Location m) const noexcept
		{
			if (is_anything_forbidden_for(get_own_sign()))
				return pattern_calculator.isForbidden(get_own_sign(), m.row, m.col);
			return false;
		}
		bool ThreatGenerator::is_opponent_3x3_fork_forbidden(Location m) const noexcept
		{
			if (is_anything_forbidden_for(get_opponent_sign()))
				return pattern_calculator.isForbidden(get_opponent_sign(), m.row, m.col);
			return false;
		}
		bool ThreatGenerator::is_anything_forbidden_for(Sign sign) const noexcept
		{
			return (game_config.rules == GameRules::RENJU) and (sign == Sign::CROSS);
		}
		const std::vector<Location>& ThreatGenerator::get_copy(const std::vector<Location> &list)
		{
			temporary_list = list;
			return temporary_list;
		}
		bool ThreatGenerator::is_move_valid(Move m) const noexcept
		{
			return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
		}

	}
} /* namespace ag */
