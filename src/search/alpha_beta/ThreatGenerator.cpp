/*
 * ThreatGenerator.cpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/ThreatGenerator.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <cassert>
#include <x86intrin.h>

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
	void get_union(ShortVector<Location, N> &lhs, const ShortVector<Location, M> &rhs) noexcept
	{
		for (int i = 0; i < rhs.size(); i++)
			if (not lhs.contains(rhs[i]))
				lhs.add(rhs[i]);
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
	ThreatGenerator::ThreatGenerator(GameConfig gameConfig, PatternCalculator &calc) :
			game_config(gameConfig),
			pattern_calculator(calc),
			moves(gameConfig.rows, gameConfig.cols),
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols)
	{
		temporary_list.reserve(64);
		forbidden_moves_cache.reserve(32);
	}
	Score ThreatGenerator::generate(ActionList &actions, GeneratorMode mode)
	{
		assert(this->actions == nullptr);
		this->actions = &actions;
		moves.fill(false);
		forbidden_moves_cache.clear();

		actions.clear();
		Result result = try_draw_in_1();
		if (result.must_continue)
			result = try_win_in_1();
		if (mode >= GeneratorMode::STATIC)
		{
			if (result.must_continue)
				result = defend_loss_in_2();
			if (result.must_continue)
				result = try_win_in_3();
			if (result.must_continue)
				result = defend_loss_in_4();
			if (result.must_continue)
				result = try_win_in_5();
		}
		if (result.must_continue and mode >= GeneratorMode::VCF)
		{
			const int count = add_own_half_open_fours();
			if (count > 0)
				actions.has_initiative = true;
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
				assert(false); // the move must be contained in the action list
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
		{ // we have to exclude moves that are forbidden
			int i = 0;
			while (i < result.size())
			{
				if (is_forbidden(get_own_sign(), result[i]))
					result.remove(i);
				else
					i++;
			}
		}
		else
		{
			if (is_anything_forbidden_for(get_opponent_sign()))
			{ // in renju there may be an additional defensive move against cross (black) open four that is blocked by a forbidden move on one end
			  // it is not very frequent (approximately 1 in 1000 positions) but we must check this in order to have correct results
				const PatternType pt = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move.row, move.col, dir);
				if (pt == PatternType::OPEN_4)
				{
					const uint32_t raw = pattern_calculator.getExtendedPatternAt(move.row, move.col, dir);
					int type = 0;
					if ((raw & 65520u) == 1344u)
						type = -1; // detected pattern '_XXX!_', if an empty spot on the left is forbidden then the spot on the right is also a defensive move
					if ((raw & 4193280u) == 344064u)
						type = +1; // detected pattern '_!XXX_', like above but flipped horizontally

					if (type != 0)
					{ // we confirmed that the type of open four is the 'straight four' one
						const Location loc = shiftInDirection(dir, 4 * type, move); // get position of the other side of the open four (4 spots away)
						if (is_forbidden(get_opponent_sign(), loc))
							result.add(shiftInDirection(dir, -1 * type, move)); // add additional defensive move, next to the existing one
					}
				}
			}
		}
		return result;
	}
	ThreatGenerator::Result ThreatGenerator::try_draw_in_1()
	{
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		if (pattern_calculator.getCurrentDepth() + 1 >= number_of_moves_for_draw)
		{ // next move will end the game with a draw
			actions->baseline_score = Score::draw();
			if (is_anything_forbidden_for(get_own_sign()))
			{ // only if there can be any forbidden move, so in renju and for cross (black) player
				temporary_list.clear();
				// loop over all legal moves, excluding those that are obviously forbidden (overlines and 4x4 forks)
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
									return Result::canStopNow(Score::draw());
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
				// overall it is extremely unlikely that we really had to get here to find a draw (less than 1 in 10.000.000 positions)
				for (auto move = temporary_list.begin(); move < temporary_list.end(); move++)
				{
					if (is_forbidden(get_own_sign(), *move))
						add_move<EXCLUDE_DUPLICATE>(*move, Score::loss_in(1));
					else
					{ // found non-forbidden 3x3 fork, can play it to draw
						add_move<EXCLUDE_DUPLICATE>(*move, Score::draw());
						return Result::canStopNow(Score::draw());
					}
				}
				return Result::canStopNow(Score::loss_in(1)); // could not find any non-forbidden move
			}
			else
			{ // if there are no forbidden moves, just find the first empty spot on board
				for (int row = 0; row < game_config.rows; row++)
					for (int col = 0; col < game_config.cols; col++)
						if (pattern_calculator.signAt(row, col) == Sign::NONE)
						{ // found at least one move that leads to draw, can stop now
							add_move<EXCLUDE_DUPLICATE>(Location(row, col), Score::draw());
							return Result::canStopNow(Score::draw());
						}
				return Result::canStopNow(Score::draw()); // the game ends up with a draw even if there is no move to play
			}
		}
		return Result::mustContinue();
	}
	ThreatGenerator::Result ThreatGenerator::try_win_in_1()
	{
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		const std::vector<Location> &own_fives = get_own_threats(ThreatType::FIVE);
		if (own_fives.size() > 0)
		{ // not often (approximately 1 in 600 positions, in renju 1 in 200)
			actions->has_initiative = true;
			add_moves<EXCLUDE_DUPLICATE>(own_fives, Score::win_in(1));
			return Result::canStopNow(Score::win_in(1));
		}
		return Result::mustContinue();
	}
	ThreatGenerator::Result ThreatGenerator::defend_loss_in_2()
	{
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		if (get_opponent_threats(ThreatType::FIVE).size() == 0)
			return Result();

		// we must copy as the check for forbidden moves may reorder the elements in the original list
		const std::vector<Location> &opponent_fives = get_copy_of(get_opponent_threats(ThreatType::FIVE));
		actions->must_defend = true;
		actions->baseline_score = Score::loss_in(2);

		DefensiveMoves defensive_moves;
		for (auto move = opponent_fives.begin(); move < opponent_fives.end(); move++)
		{ // loop over threats of five to find a set of moves that refute them all
			const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
			const Direction direction = group.findDirectionOf(PatternType::FIVE);
			const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);

			defensive_moves.get_intersection_with(tmp);
			if (defensive_moves.is_empty())
			{ // surprisingly not common (approximately 1 in 3000 positions)
				add_moves<EXCLUDE_DUPLICATE>(opponent_fives, Score::loss_in(2)); // we must always produce some moves, even if the threat is not refutable
				return Result::canStopNow(Score::loss_in(2));
			}
		}

		Score best_score = Score::min_value();
		for (auto move = defensive_moves.get_list().begin(); move < defensive_moves.get_list().end(); move++)
		{
			Score response_score;
			switch (get_own_threat_at(move->row, move->col))
			{
				case ThreatType::FORK_3x3:
				{
					assert(is_forbidden(get_own_sign(), *move) == false); // forbidden moves should have been excluded earlier
					if (is_anything_forbidden_for(get_own_sign()))
					{ // relevant only for renju rule and cross (black) player
						const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
						if (group.contains(PatternType::OPEN_4)) // there is an open four hidden inside a legal 3x3 fork
							response_score = Score::win_in(3); // extremely rare (approximately 1 in 60.000.000 positions)
					}
					else
					{ // for rules other than renju
						if (not pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour()) // we have 3x3 fork while the opponent has no four
							response_score = Score::win_in(5); // rare (approximately 1 in 4000 positions)
					}
					break;
				}
				case ThreatType::FORK_4x3:
				{ // rare (approximately 1 in 700 positions)
					const Score solution = try_solve_own_fork_4x3(*move);
					if (solution.isWin())
						response_score = solution; // found a surely winning 4x3 fork
					break;
				}
				case ThreatType::FORK_4x4:
				{ // rare (approximately 1 in 3000 positions)
					assert(is_anything_forbidden_for(get_own_sign()) == false); // forbidden moves should have been excluded earlier
					response_score = Score::win_in(3);
					break;
				}
				case ThreatType::OPEN_4:
				{ // rare (approximately 1 in 500 positions)
					response_score = Score::win_in(3);
					break;
				}
				default:
				{
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
					if (group.contains(PatternType::HALF_OPEN_4))
					{ // quite common (approximately 1 in 40 positions)
						actions->has_initiative = true;  // there is some half-open four to make in a response (may help gain initiative)
						// in renju it is possible that this half-open four would create a foul attack and potential win
						// it is rare (approximately 1 in 9000 positions) and would require separate method to check so we skip it
					}
					break;
				}
			}
			if (response_score.isWin()) // overall it is not common (approximately 1 in 250 positions)
				actions->has_initiative = true;
			add_move<EXCLUDE_DUPLICATE>(*move, response_score);
			best_score = std::max(best_score, response_score);
		}
		return Result::canStopNow(best_score); // although there is no proven score, we don't have to continue checking another threats
	}
	ThreatGenerator::Result ThreatGenerator::try_win_in_3()
	{
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		int hidden_open_four_count = 0;
		if (is_anything_forbidden_for(get_own_sign()))
		{ // in renju there might be an open four hidden inside a legal 3x3 fork
		  // it is very rare (approximately 1 in 500.000 positions) but we must check this in order to have correct results

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const std::vector<Location> &opp_fork_3x3 = get_copy_of(get_own_threats(ThreatType::FORK_3x3));
			for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
			{
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
				if (group.contains(PatternType::OPEN_4) and not is_forbidden(get_own_sign(), *move))
				{
					hidden_open_four_count++;
					add_move<EXCLUDE_DUPLICATE>(*move, Score::win_in(3));
				}
			}
		}

		const std::vector<Location> &own_open_four = get_own_threats(ThreatType::OPEN_4);
		add_moves<EXCLUDE_DUPLICATE>(own_open_four, Score::win_in(3));
		if (hidden_open_four_count + own_open_four.size() > 0)
		{ // frequent in renju (1 in 20) but rare otherwise (approximately 1 in 450 positions)
			actions->has_initiative = true;
			return Result::canStopNow(Score::win_in(3));
		}

		const std::vector<Location> &own_fork_4x4 = get_own_threats(ThreatType::FORK_4x4);
		if (own_fork_4x4.size() > 0 and not is_anything_forbidden_for(get_own_sign()))
		{ // rare (approximately 1 in 1700 positions)
			actions->has_initiative = true;
			add_moves<EXCLUDE_DUPLICATE>(own_fork_4x4, Score::win_in(3));
			return Result::canStopNow(Score::win_in(3));
		}

		if (is_anything_forbidden_for(get_opponent_sign()))
		{ // in renju, we might be able to create a threat of five where the opponent will have a forbidden move - only for circle (white) player
		  // it is quite frequent (approximately 1 in 300 positions) and we can successfully solve ~98% of such positions

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const std::vector<Location> &own_half_open_four = get_copy_of(get_own_threats(ThreatType::HALF_OPEN_4));
			for (auto move = own_half_open_four.begin(); move < own_half_open_four.end(); move++)
			{ // we take advantage of the fact that in renju, half-open fours always come in pairs, for example !XX!X
			  // we can loop over available half-open fours and check if any of them is on the spot forbidden for the opponent
			  // only then we check its defensive moves to find that other half-open four from the pair that would create a foul attack
			  // such approach saves a lot of lookups to the defensive moves table which are costly
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
				assert(group.count(PatternType::HALF_OPEN_4) == 1);
				const Direction dir = group.findDirectionOf(PatternType::HALF_OPEN_4);

				bool is_winning = false;
				switch (get_opponent_threat_at(move->row, move->col))
				{
					default:
						break;
					case ThreatType::FORK_3x3:
					{ // if own half-open four is in the same direction as opponent open three, they may influence each other
					  // it is quite rare (approximately 1 in 100.000 positions) and very complicated to check so we skip it
					  // and anyway, by creating such half-open four we would most likely make the 3x3 fork legal, or remove it at all
						const DirectionGroup<PatternType> tmp = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
						if (tmp[dir] != PatternType::OPEN_3 and is_forbidden(get_opponent_sign(), *move))
							is_winning = true;
						break;
					}
					case ThreatType::FORK_4x4:
					case ThreatType::OVERLINE:
						is_winning = true;
						break;
				}

				if (is_winning)
				{
					const ShortVector<Location, 6> tmp = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move->row, move->col, dir); // we intentionally want to have all moves, including forbidden ones
					assert(tmp.size() == 2); // in renju rule, there should be exactly two defensive moves to half-open four
					// one of them will be at the same spot where we have just created this threat, we want to check that other one
					const Location original = (tmp[0] == *move) ? tmp[1] : tmp[0];
					add_move<EXCLUDE_DUPLICATE>(original, Score::win_in(3));
					return Result::canStopNow(Score::win_in(3));
				}
			}
			// there are also half-open fours hidden inside legal 3x3 forks, but they are less than 0.5% of all half-open fours, so we don't check it here
		}
		return Result::mustContinue();
	}
	ThreatGenerator::Result ThreatGenerator::defend_loss_in_4()
	{
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		const bool has_any_four = pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour();
//			if (pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour())
//			{ // any four that we can make may help to gain initiative
//				actions->has_initiative = true;
//				const Score best_score = add_own_4x3_forks();
//				if (best_score.isWin())
//					return Result::canStopNow(best_score);
//				else
//					add_own_half_open_fours(); // it makes sense to add other threats only if there is no winning 4x3 fork
//			}
		actions->baseline_score = Score::loss_in(4); // will be reverted at the end if there are no threats

		if (game_config.rules != GameRules::RENJU)
		{
			// in order to find a minimal number of defensive moves we must calculate an intersection of defensive moves to all individual threats
			// if there are any moves that occur as a refutation to all threats we can make them
			// otherwise, if there is no move that can defend against all threats it is a loss in 4 (provided that we can't make any own fours)
			DefensiveMoves defensive_moves;

			const std::vector<Location> &opp_open_four = get_opponent_threats(ThreatType::OPEN_4);
			for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
			{
				actions->must_defend = true;
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
				const Direction direction = group.findDirectionOf(PatternType::OPEN_4);
				const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);

				defensive_moves.get_intersection_with(tmp);
				if (defensive_moves.is_empty() and not has_any_four)
				{ // we already proved that the threats are not refutable
					add_moves<EXCLUDE_DUPLICATE>(opp_open_four, Score::loss_in(4)); // we must always produce some moves, even if the threat is not refutable
					return Result::canStopNow(Score::loss_in(4));
				}
			}

			const std::vector<Location> &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
			ShortVector<Location, 24> storage;
			for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
			{
				actions->must_defend = true;
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);

				// at first resolve open and double fours, as they all must be refuted by a single move
				for (Direction dir = 0; dir < 4; dir++)
					if (group[dir] == PatternType::OPEN_4 or group[dir] == PatternType::DOUBLE_4)
						defensive_moves.get_intersection_with(get_defensive_moves(*move, dir));

				// then resolve half-open fours
				// this case is more complicated as we actually only need to refute all but one of them
				// we would need to check all combinations and pick the one that has the largest intersection with the defensive moves
				// and then test it against all combinations for other forks.
				// 4x4 forks containing half-open fours are quite frequent (approximately than 1 in 60 positions)
				// but it's too complicated to check exactly, we approximate the result by taking union of defensive moves to all half-open fours
				// it produces more moves than necessary but at least it will not overlook any
				if (group.count(PatternType::HALF_OPEN_4) > 0)
				{
					storage.clear();
					for (Direction dir = 0; dir < 4; dir++)
						if (group[dir] == PatternType::HALF_OPEN_4)
							get_union(storage, get_defensive_moves(*move, dir));
					defensive_moves.get_intersection_with(storage);
				}

				if (defensive_moves.is_empty() and not has_any_four)
				{ // we already proved that the threats are not refutable (approximately 1 in 700 positions)
					add_moves<EXCLUDE_DUPLICATE>(opp_fork_4x4, Score::loss_in(4)); // we must always produce some moves, even if the threat is not refutable
					return Result::canStopNow(Score::loss_in(4));
				}
			}
			add_moves<EXCLUDE_DUPLICATE>(defensive_moves.get_list().begin(), defensive_moves.get_list().end());
		}
		else
		{ // in renju rule there are too complex dependencies between defensive and forbidden moves, this is why we add all defensive moves
			{ // artificial scope so that the list below is not used later
				const std::vector<Location> &opp_open_four = get_copy_of(get_opponent_threats(ThreatType::OPEN_4));
				for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
				{
					actions->must_defend = true;
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					const Direction direction = group.findDirectionOf(PatternType::OPEN_4);
					const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);
					add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
				}
			}

			if (is_anything_forbidden_for(get_opponent_sign()))
			{ // in renju there might be an open four hidden inside a legal 3x3 fork
			  // it is very rare (approximately 1 in 400.000 positions) but we must do this in order to have correct results

				// we must copy as the check for forbidden moves may reorder the elements in the original list
				const std::vector<Location> &opp_fork_3x3 = get_copy_of(get_opponent_threats(ThreatType::FORK_3x3));
				for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
				{
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					if (group.contains(PatternType::OPEN_4) and not is_forbidden(get_opponent_sign(), *move))
					{
						actions->must_defend = true;
						const Direction direction = group.findDirectionOf(PatternType::OPEN_4);
						const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);
						add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
					}
				}
			}

			if (not is_anything_forbidden_for(get_opponent_sign()))
			{
				// we must copy as the check for forbidden moves may reorder the elements in the original list
				const std::vector<Location> &opp_fork_4x4 = get_copy_of(get_opponent_threats(ThreatType::FORK_4x4));
				for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
				{
					actions->must_defend = true;
					const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
					for (Direction dir = 0; dir < 4; dir++)
						if (is_a_four(group[dir]))
						{
							const ShortVector<Location, 6> tmp = get_defensive_moves(*move, dir);
							add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end());
						}
				}
			}
		}
		if (actions->must_defend)
		{ // TODO actually all those moves should be added before any defensive ones, but as for now it would be tricky to implement
			actions->has_initiative = has_any_four;
			const Score best_score = add_own_4x3_forks();
			if (best_score.isWin())
				return Result::canStopNow(best_score);
			add_own_half_open_fours(); // it makes sense to add other threats only if there is no winning 4x3 fork
			return Result::canStopNow();
		}
		else
			actions->baseline_score = Score();
		return Result::mustContinue();
	}
	ThreatGenerator::Result ThreatGenerator::try_win_in_5()
	{
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		Score best_score = add_own_4x3_forks();

		if (not is_anything_forbidden_for(get_own_sign()))
		{ // in renju, for cross (black) player it is possible that in 3x3 fork some open 3 can't be converted to a four because they will be forbidden for some reason
		  // but it is too complicated to solve statically, so we check only for circle (white) player
			const int open_4_count = get_opponent_threats(ThreatType::OPEN_4).size();
			const int fork_4x4_count = is_anything_forbidden_for(get_opponent_sign()) ? 0 : get_opponent_threats(ThreatType::FORK_4x4).size();
			const int fork_4x3_count = get_opponent_threats(ThreatType::FORK_4x3).size();
			const int half_open_4_count = get_opponent_threats(ThreatType::HALF_OPEN_4).size();
			if ((open_4_count + fork_4x4_count + fork_4x3_count + half_open_4_count) == 0)
			{ // opponent has no four to make
				const std::vector<Location> &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
				add_moves<EXCLUDE_DUPLICATE>(own_fork_3x3, Score::win_in(5));
				if (own_fork_3x3.size() > 0) // it happens quite often (approximately 1 in 150 positions)
					best_score = std::max(best_score, Score::win_in(5));
			}
		}

		if (best_score.isWin())
		{
			actions->has_initiative = true;
			return Result::canStopNow(best_score);
		}
		return Result::mustContinue();
	}
	Score ThreatGenerator::add_own_4x3_forks()
	{
		Score result;
		const std::vector<Location> &own_fork_4x3 = get_own_threats(ThreatType::FORK_4x3);
		for (auto iter = own_fork_4x3.begin(); iter < own_fork_4x3.end(); iter++)
		{
			const Score solution = try_solve_own_fork_4x3(*iter);
			add_move<OVERRIDE_DUPLICATE>(*iter, solution); // the move might have been added earlier as defensive move, so we must override its score
			if (solution.isProven())
				result = std::max(result, solution);
		}
		return result;
	}
	int ThreatGenerator::add_own_half_open_fours()
	{
		int hidden_count = 0;
		if (is_anything_forbidden_for(get_own_sign()))
		{ // in renju rule there might be a half-open four hidden inside a legal 3x3 fork - only for cross (black) player
		  // it is rare (approximately 1 in 60.000 positions) but cheap to check

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const std::vector<Location> &own_fork_3x3 = get_copy_of(get_own_threats(ThreatType::FORK_3x3));
			for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
			{
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
				if (group.contains(PatternType::HALF_OPEN_4) and not is_forbidden(get_own_sign(), *move))
				{
					add_move<EXCLUDE_DUPLICATE>(*move);
					hidden_count++;
				}
			}
		}

		// in theory we can exclude half-open fours where the opponent can create a winning threat in response
		// they are rare (less than 0.5% of all half-open fours) and the check is very costly so we don't do this
//			const std::vector<Location> &own_half_open_fours = get_own_threats(ThreatType::HALF_OPEN_4);
//			for (auto move = own_half_open_fours.begin(); move < own_half_open_fours.end(); move++)
//			{
//				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
//				const Direction dir = group.findDirectionOf(PatternType::HALF_OPEN_4);
//				ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move->row, move->col, dir);
//				defensive_moves.remove(*move); // excluding move at the spot that we occupied to create this threat
//
//				ThreatType best_opponent_threat = ThreatType::NONE;
//				for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
//					best_opponent_threat = std::max(best_opponent_threat, get_opponent_threat_at(iter->row, iter->col));
//
//				switch (best_opponent_threat)
//				{
//					default:
//					{
//						hidden_count++;
//						add_move<EXCLUDE_DUPLICATE>(*move);
//						break;
//					}
//					case ThreatType::FORK_4x4:
//					case ThreatType::OPEN_4:
//					case ThreatType::FIVE:
//						break;
//				}
//			}
//			return hidden_count;
		add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::HALF_OPEN_4)); // half-open four is a threat of win in 1
		return hidden_count + get_own_threats(ThreatType::HALF_OPEN_4).size();
	}
	Score ThreatGenerator::try_solve_own_fork_4x3(Location move)
	{
		assert(get_own_threat_at(move.row, move.col) == ThreatType::FORK_4x3);
		if (is_anything_forbidden_for(get_own_sign())) // unfortunately it is very often (approximately i in 30 positions)
			return Score(); // there is a possibility that inside the three there will later be a forbidden move, it's too complicated to check statically

		const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move.row, move.col);
		const Direction direction = group.findDirectionOf(PatternType::HALF_OPEN_4);
		ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move.row, move.col, direction);
		defensive_moves.remove(move); // excluding move at the spot that we occupied to create this threat

		ThreatType best_opponent_threat = ThreatType::NONE;
		for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
		{
			const ThreatType tt = get_opponent_threat_at(iter->row, iter->col);
			// in renju we don't want to consider forbidden responses but simple max would include them and potentially hide allowed responses (which have lower level threats)
			// but 3x3 fork can be included as it does not form a danger for us anyway
			if ((tt != ThreatType::FORK_4x4 and tt != ThreatType::OVERLINE) or not is_anything_forbidden_for(get_opponent_sign()))
				best_opponent_threat = std::max(best_opponent_threat, tt);
		}

		switch (best_opponent_threat)
		{
			default:
			case ThreatType::NONE:
			case ThreatType::HALF_OPEN_3:
			case ThreatType::OPEN_3:
				return Score::win_in(5); // opponent cannot make any four in response to our threat
			case ThreatType::FORK_3x3: // in renju, rarely (approximately 1 in 7000 positions) it happens that the win is in 3 plys instead of in 5, because the 3x3 fork is forbidden
				return Score::win_in(5); // but it is not worth checking, it is a win anyway, doesn't matter if longer or shorter
			case ThreatType::HALF_OPEN_4: // moderately rare (approximately 1 in 300 positions)
			case ThreatType::FORK_4x3: // rare (approximately 1 in 5000 positions)
				return Score(); // too complicated to check statically
			case ThreatType::FORK_4x4: // rare (approximately 1 in 4500 positions)
			case ThreatType::OPEN_4:
				return Score::loss_in(4); // opponent will create a 'win in 3' type of threat
			case ThreatType::FIVE: // actually this should never happen, as if the opponent has any five it should have been handled earlier in 'defend_loss_in_2()'
			case ThreatType::OVERLINE:
				return Score::loss_in(2); // opponent will create winning threat in response
		}
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
	bool ThreatGenerator::is_anything_forbidden_for(Sign sign) const noexcept
	{
		return (game_config.rules == GameRules::RENJU) and (sign == Sign::CROSS);
	}
	bool ThreatGenerator::is_forbidden(Sign sign, Location loc) const noexcept
	{
		if (is_anything_forbidden_for(sign))
		{ // caching forbidden moves is not that much useful, it has about 7% hit rate
		  // and saves less than 0.04% of pattern recalculations
			for (size_t i = 0; i < forbidden_moves_cache.size(); i++)
				if (forbidden_moves_cache[i].first == loc)
					return forbidden_moves_cache[i].second;
			const bool result = pattern_calculator.isForbidden(sign, loc.row, loc.col);
			forbidden_moves_cache.push_back(std::pair<Location, bool>(loc, result));
			return result;
		}
		return false;
	}
	const std::vector<Location>& ThreatGenerator::get_copy_of(const std::vector<Location> &list)
	{
		temporary_list = list;
		return temporary_list;
	}
	bool ThreatGenerator::is_move_valid(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

} /* namespace ag */
