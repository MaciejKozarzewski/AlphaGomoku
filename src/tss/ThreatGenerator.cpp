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

	bool contains(const std::vector<Location> &list, Move m) noexcept
	{
		return std::find(list.begin(), list.end(), m.location()) != list.end();
	}
	bool is_a_three(PatternType pt) noexcept
	{
		return pt == PatternType::HALF_OPEN_3 or pt == PatternType::OPEN_3;
	}
	bool is_a_four(PatternType pt) noexcept
	{
		return pt == PatternType::HALF_OPEN_4 or pt == PatternType::OPEN_4 or pt == PatternType::DOUBLE_4;
	}

	int get(uint32_t line, int idx) noexcept
	{
		return (line >> idx) & 3;
	}
	constexpr bool is_overline_allowed(GameRules rules, Sign attackerSign) noexcept
	{
		return (rules == GameRules::FREESTYLE) or (rules == GameRules::RENJU and attackerSign == Sign::CIRCLE) or (rules == GameRules::CARO6);
	}
	constexpr bool is_blocked_allowed(GameRules rules, Sign sign) noexcept
	{
		return rules != GameRules::CARO5 and rules != GameRules::CARO6;
	}
	class IsAThree
	{
			static constexpr std::array<uint32_t, 48> patterns = { 5376, 21504, 86016, 4416, 70656, 282624, 5184, 20736, 331776, 5376, 21504, 86016,
					5376, 21504, 86016, 5184, 20736, 331776, 4416, 70656, 282624, 5376, 21504, 86016, 5136, 20544, 1314816, 4368, 69888, 1118208,
					5184, 20736, 331776, 4176, 267264, 1069056, 4416, 70656, 282624, 5376, 21504, 86016, 0, 0, 0, 0, 0, 0 };
			static constexpr std::array<uint32_t, 48> masks = { 262080, 1048320, 4193280, 65520, 1048320, 4193280, 65520, 262080, 4193280, 65520,
					262080, 1048320, 16368, 65472, 261888, 16368, 65472, 1047552, 16368, 261888, 1047552, 65472, 261888, 1047552, 16368, 65472,
					4190208, 16368, 261888, 4190208, 65472, 261888, 4190208, 16368, 1047552, 4190208, 65472, 1047552, 4190208, 261888, 1047552,
					4190208, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
			static constexpr std::array<uint8_t, 96> side_offsets = { 4, 18, 6, 20, 8, 22, 2, 16, 6, 20, 8, 22, 2, 16, 4, 18, 8, 22, 2, 16, 4, 18, 6,
					20, 2, 14, 4, 16, 6, 18, 2, 14, 4, 16, 8, 20, 2, 14, 6, 18, 8, 20, 4, 16, 6, 18, 8, 20, 2, 14, 4, 16, 10, 22, 2, 14, 6, 18, 10,
					22, 4, 16, 6, 18, 10, 22, 2, 14, 8, 20, 10, 22, 4, 16, 8, 20, 10, 22, 6, 18, 8, 20, 10, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			Sign attacker_sign;
			bool allow_overline = false;
			bool allow_blocked = false;
		public:
			IsAThree(GameRules rules, Sign attackerSign) :
					attacker_sign(attackerSign),
					allow_overline(is_overline_allowed(rules, attacker_sign)),
					allow_blocked(is_blocked_allowed(rules, attacker_sign))
			{
			}
			bool operator()(uint32_t pattern) const noexcept
			{
				if ((pattern & 12288u) != 0)
					return false; // center spot is not empty
				if (attacker_sign == Sign::CIRCLE)
					pattern = ((pattern >> 1) & 0x55555555) | ((pattern & 0x55555555) << 1); // invert color
				pattern |= (static_cast<uint32_t>(Sign::CROSS) << 12);
#ifdef __AVX2__
				const __m256i val = _mm256_set1_epi32(pattern);
				for (int i = 0; i < 48; i += 16)
				{
					const __m256i p0 = _mm256_loadu_si256((__m256i*) (patterns.data() + i));
					const __m256i p1 = _mm256_loadu_si256((__m256i*) (patterns.data() + i + 8));
					const __m256i m0 = _mm256_loadu_si256((__m256i*) (masks.data() + i));
					const __m256i m1 = _mm256_loadu_si256((__m256i*) (masks.data() + i + 8));
					const __m256i cmp0 = _mm256_cmpeq_epi32(p0, _mm256_and_si256(val, m0));
					const __m256i cmp1 = _mm256_cmpeq_epi32(p1, _mm256_and_si256(val, m1));
					const __m256i tmp = _mm256_permute4x64_epi64(_mm256_packs_epi32(cmp0, cmp1), 0b11'01'10'00);
					const int mask = _mm256_movemask_epi8(tmp);
					if (mask != 0)
						return check_sides(pattern, i + _bit_scan_forward(mask) / 2);
				}
#else
				for (int i = 0; i < 42; i++)
					if ((pattern & masks[i]) == patterns[i])
						return check_sides(pattern, i);
#endif
				return false;
			}
		private:
			bool check_sides(uint32_t pattern, int idx) const noexcept
			{
				const Sign first = static_cast<Sign>(get(pattern, side_offsets[2 * idx + 0]));
				const Sign last = static_cast<Sign>(get(pattern, side_offsets[2 * idx + 1]));
				if (not allow_overline and (first == Sign::CROSS or last == Sign::CROSS))
					return false; // check if any stone on the sides is in the color of an attacker
				if (not allow_blocked and (first == Sign::CIRCLE and last == Sign::CIRCLE))
					return false; // check if both stones on the sides are in the color of the defender
				return true;
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
		Score ThreatGenerator::generate(ActionList &actions, int level)
		{
			assert(this->actions == nullptr);
			this->actions = &actions;
			moves.fill(false);

			actions.clear();
			Result result = try_win_in_1();
			if (result.can_continue)
				result = try_draw_in_1();
			if (result.can_continue and level >= 1)
				result = defend_loss_in_2();
			if (result.can_continue and level >= 1)
				result = try_win_in_3();
			if (result.can_continue and level >= 1)
				result = defend_loss_in_4();
			if (result.can_continue and level >= 1)
				result = try_win_in_5();
			if (result.can_continue and level >= 2)
			{
				const int count = add_own_half_open_fours();
				if (count > 0)
					actions.has_initiative = true;
//				add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::OPEN_3));
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
		int ThreatGenerator::create_defensive_moves(Location move, Direction dir)
		{
			const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), move.row, move.col, dir);
			if (is_anything_forbidden_for(get_own_sign()))
			{
				int count = 0;
				for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
					if (not pattern_calculator.isForbidden(get_own_sign(), iter->row, iter->col))
					{
						add_move<EXCLUDE_DUPLICATE>(*iter);
						count++;
					}
				return count;
			}
			else
			{
				int count = defensive_moves.size();
				for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
					add_move<EXCLUDE_DUPLICATE>(*iter);

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
								add_move<EXCLUDE_DUPLICATE>(loc2);
								count++;
							}
						}
					}
				}
				return count;
			}
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
			switch (opponent_fives.size())
			{
				case 0: // no fives
					return Result();
				case 1: // single five
				{
					actions->must_defend = true;
					// check if any of the defensive moves is at the same time a win for us (but only if there is at most single five for the opponent)
					const Location five_move = opponent_fives[0];
					if (is_caro_rule())
					{ // in caro there may be multiple defensive moves to the threat of five
						const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), five_move.row,
								five_move.col);
						assert(group.count(PatternType::FIVE) == 1); // TODO I think it's not possible to create two fives in different directions at the same spot
						const Direction dir = group.findDirectionOf(PatternType::FIVE);
						const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), five_move.row,
								five_move.col, dir);

						// check if there is any 4x4 fork or an open 4
						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
							if (get_own_threat_at(iter->row, iter->col) == ThreatType::FORK_4x4
									or get_own_threat_at(iter->row, iter->col) == ThreatType::OPEN_4)
							{
								actions->has_initiative = true;
								add_move<EXCLUDE_DUPLICATE>(*iter, Score::win_in(3));
								return Result(false, Score::win_in(3));
							}
						// check if there is any winning 4x3 fork
						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
							if (get_own_threat_at(iter->row, iter->col) == ThreatType::FORK_4x3)
							{
								const Score solution = try_solve_own_fork_4x3(*iter);
								if (solution.isWin())
								{
									actions->has_initiative = true;
									add_move<EXCLUDE_DUPLICATE>(*iter, solution);
									return Result(false, solution);
								}
							}
						if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour())
						{ // if the opponent has no fours, check if there is a 3x3 fork
							for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
								if (get_own_threat_at(iter->row, iter->col) == ThreatType::FORK_3x3)
								{
									actions->has_initiative = true;
									add_move<EXCLUDE_DUPLICATE>(*iter, Score::win_in(5));
									return Result(false, Score::win_in(5));
								}
						}
						// if no winning response was found, check if there is a half open four to make (may help to gain initiative)
						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
						{
							const DirectionGroup<PatternType> tmp = pattern_calculator.getPatternTypeAt(get_own_sign(), iter->row, iter->col);
							if (tmp.contains(PatternType::HALF_OPEN_4))
								actions->has_initiative = true;
						}
						// in the end, add defensive moves
						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
							add_move<EXCLUDE_DUPLICATE>(*iter);
					}
					else
					{ // for rules other than caro there is exactly one defensive move to the threat of five (the same move that creates a five)
						const Location response(five_move);
						switch (get_own_threat_at(response.row, response.col))
						{
							case ThreatType::FORK_3x3:
							{
								if (is_own_3x3_fork_forbidden(response))
								{ // relevant only for renju rule
									add_move<EXCLUDE_DUPLICATE>(response, Score::loss_in(2)); // the defensive move is forbidden -> loss
									return Result(false, Score::loss_in(2));
								}
								else
								{
									if (is_anything_forbidden_for(get_own_sign()))
									{ // relevant only for renju rule
										const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_own_sign(), response.row,
												response.col);
										if (patterns.contains(PatternType::OPEN_4))
										{ // there is an open four hidden inside a legal 3x3 fork
											actions->has_initiative = true;
											add_move<EXCLUDE_DUPLICATE>(response, Score::win_in(3));
											return Result(false, Score::win_in(3));
										}
									}
									if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour())
									{ // we have 3x3 fork while the opponent has no four
										actions->has_initiative = true;
										add_move<EXCLUDE_DUPLICATE>(response, Score::win_in(5));
										return Result(false, Score::win_in(5));
									}
								}
								break;
							}
							case ThreatType::FORK_4x3:
							{
								const Score solution = try_solve_own_fork_4x3(response);
								if (solution.isWin())
								{ // found a surely winning 4x3 fork
									actions->has_initiative = true;
									add_move<EXCLUDE_DUPLICATE>(response, solution);
									return Result(false, solution);
								}
								break;
							}
							case ThreatType::FORK_4x4:
							{
								if (is_own_4x4_fork_forbidden())
								{ // relevant only for renju rule
									add_move<EXCLUDE_DUPLICATE>(response, Score::loss_in(2)); // the defensive move is forbidden -> loss
									return Result(false, Score::loss_in(2));
								}
								else
								{
									actions->has_initiative = true;
									add_move<EXCLUDE_DUPLICATE>(response, Score::win_in(3));
									return Result(false, Score::win_in(3));
								}
							}
							case ThreatType::OPEN_4:
							{
								actions->has_initiative = true;
								add_move<EXCLUDE_DUPLICATE>(response, Score::win_in(3));
								return Result(false, Score::win_in(3));
							}
							default:
								break;
						}
						const DirectionGroup<PatternType> tmp = pattern_calculator.getPatternTypeAt(get_own_sign(), response.row, response.col);
						if (tmp.contains(PatternType::HALF_OPEN_4))
							actions->has_initiative = true;  // there is some half-open four to make in a response (may help gain initiative)

						add_move<EXCLUDE_DUPLICATE>(response); // in the end, add defensive move
					}
					return Result(false); // although there is no proven score, we don't have to continue checking another threats
				}
				default: // multiple fives
				{
					if (is_caro_rule())
					{ // in caro rule it is possible to block two threats of five with a single move, for example "!XXXX_O"
						Location found_move(-1, -1);
						bool flag = true;
						for (auto five_move = opponent_fives.begin(); five_move < opponent_fives.end() and flag; five_move++)
						{ // loop over all opponent fives to find defensive moves
							const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), five_move->row,
									five_move->col);
							const Direction dir = group.findDirectionOf(PatternType::FIVE);
							const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), five_move->row,
									five_move->col, dir);
							for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
							{ // loop over defensive moves to check if they all are at the same spot
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
							return Result(false); // we have found single defensive move to refute all fives
					}
					// if we got here it means that either:
					//  - it is caro rule and there is no single move that would defend against all opponent fives, or
					//  - it is not caro rule so multiple opponent fives are always losing
					for (auto iter = opponent_fives.begin(); iter < opponent_fives.end(); iter++)
						add_move<EXCLUDE_DUPLICATE>(*iter, Score::loss_in(2)); // just add some moves marking them as loss
					return Result(false, Score::loss_in(2));
				}
			}
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

			int opp_win_in_3_count = 0;
			{ // artificial scope so that the list below is not used later
				const std::vector<Location> &opp_open_four = get_copy(get_opponent_threats(ThreatType::OPEN_4));
				opp_win_in_3_count += opp_open_four.size();
				for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
				{
					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
					create_defensive_moves(*move, direction);
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
						opp_win_in_3_count++;
						const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
						create_defensive_moves(*move, direction);
					}
				}
			}

			const std::vector<Location> &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
			if (not is_opponent_4x4_fork_forbidden())
			{
				opp_win_in_3_count += opp_fork_4x4.size();
				for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
				{
					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					for (Direction direction = 0; direction < 4; direction++)
						if (is_a_four(patterns[direction]))
							create_defensive_moves(*move, direction);
				}
			}

			if (opp_win_in_3_count > 0)
			{
				actions->must_defend = true;
				if (pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour())
					actions->has_initiative = true;
				// open fours and 4x4 forks have been added in previous steps
				const Score best_score = add_own_4x3_forks();
				if (not best_score.isWin()) // it makes sense to add other threats only if there is no winning 4x3 fork
					add_own_half_open_fours();
				return Result(false, best_score);
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
		ThreatGenerator::Result ThreatGenerator::defend_loss_in_6()
		{
//			assert(actions->must_defend == false && actions->has_initiative == false);
//			IsAThree is_a_three(game_config.rules, get_own_sign());
//
//			int opp_4x3_fork_count = 0;
//			const std::vector<Location> &opp_fork_4x3 = get_opponent_threats(ThreatType::FORK_4x3);
//			for (auto move = opp_fork_4x3.begin(); move < opp_fork_4x3.end(); move++)
//				if (is_4x3_fork_winning(move->row, move->col, get_opponent_sign()))
//				{ // only get here if the opponent 4x3 fork is surely winning
//					opp_4x3_fork_count++;
//					const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
//					for (Direction direction = 0; direction < 4; direction++)
//					{
//						if (patterns[direction] == PatternType::HALF_OPEN_4)
//						{ // handle the direction in which a four can be made
//							create_defensive_moves(*move, direction); // mark direct defensive move to the opponent four
//							// find indirect defensive moves (those creating any kind of three or a four for us)
//							const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), move->row,
//									move->col, direction);
//							for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)	// check the surroundings of the half open four from 4x3 fork
//							{ // loop over responses to the opponent half-open four to check its neighborhood
//								for (Direction dir = 0; dir < 4; dir++) // check all directions around the response to the half-open four
//									for (int j = -4; j <= 4; j++) // check the surroundings of the response
//									{
//										const int x = iter->row + j * get_row_step(dir);
//										const int y = iter->col + j * get_col_step(dir);
//										if (0 <= x and x < game_config.rows and 0 <= y and y < game_config.cols)
//										{ // if inside board
//											const uint32_t raw = pattern_calculator.getExtendedPatternAt(x, y, dir);
//											if (is_a_three(raw))
//												add_move(Location(x, y)); // this move creates promising threat that may refute opponent 4x3 fork
////											const PatternType pt = pattern_calculator.getPatternTypeAt(get_own_sign(), x, y, dir);
////											if (is_a_three(pt))
////												add_move(Location(x, y)); // this move creates promising threat that may refute opponent 4x3 fork
//										}
//									}
//							}
//						}
//						if (patterns[direction] == PatternType::OPEN_3)
//							create_defensive_moves(*move, direction);
//					}
//				}
//
//			if (opp_4x3_fork_count > 0)
//			{
//				actions->must_defend = true;
//				if (pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour())
//					actions->has_initiative = true;
//				add_moves(get_own_threats(ThreatType::HALF_OPEN_4)); // half open four is a threat of win in 1 (may help gain initiative)
//				add_moves(get_own_threats(ThreatType::FORK_4x3)); // half open four is a threat of win in 1 (may help gain initiative)
//				// open fours and 4x4 forks have been added in previous steps
//				add_moves(get_own_threats(ThreatType::OPEN_3)); // half open four is a threat of win in 5 (may help gain initiative)
//				add_moves(get_own_threats(ThreatType::HALF_OPEN_3)); // half open four is a threat of win in 5 (may help gain initiative)
//				return Result(false);
//			}
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
								add_move<OVERRIDE_DUPLICATE>(*move);
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
		bool ThreatGenerator::is_4x3_fork_winning(int row, int col, Sign sign)
		{
			if (is_anything_forbidden_for(sign))
				return false; // there is a possibility that inside the three there will later be a forbidden move -> too complicated to check statically

			const Sign defender_sign = invertSign(sign);
			const DirectionGroup<PatternType> own_patterns = pattern_calculator.getPatternTypeAt(sign, row, col);
			const Direction direction_of_own_four = own_patterns.findDirectionOf(PatternType::HALF_OPEN_4);
			ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(defender_sign, row, col, direction_of_own_four);
			defensive_moves.remove(Location(row, col));

			ThreatType best_opponent_threat = ThreatType::NONE;
			for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
				best_opponent_threat = std::max(best_opponent_threat, pattern_calculator.getThreatAt(defender_sign, iter->row, iter->col));

			switch (best_opponent_threat)
			{
				default:
				case ThreatType::NONE:
				case ThreatType::HALF_OPEN_3:
				case ThreatType::OPEN_3:
				case ThreatType::FORK_3x3:
					return true; // opponent cannot make any four in response to our threat
				case ThreatType::HALF_OPEN_4:
				case ThreatType::FORK_4x3:
					return false; // too complicated to check statically
				case ThreatType::FORK_4x4:
					return is_anything_forbidden_for(defender_sign);
				case ThreatType::OPEN_4:
				case ThreatType::FIVE:
					return false;
				case ThreatType::OVERLINE:
					return is_anything_forbidden_for(defender_sign);
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
