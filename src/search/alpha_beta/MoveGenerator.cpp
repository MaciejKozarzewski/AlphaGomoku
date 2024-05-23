/*
 * MoveGenerator.cpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <cassert>
#include <bitset>
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

	struct PatternWrapper
	{
			uint32_t raw;
			PatternWrapper(ReducedPattern x) noexcept :
					raw(static_cast<uint32_t>(x))
			{
			}
			uint32_t operator[](int idx) const noexcept
			{
				assert(-4 <= idx && idx <= 4);
				const uint32_t shift = 2 * (idx + 4);
				return (raw >> shift) & 3u;
			}
	};
	bool is_spot_empty(int idx, PatternWrapper p) noexcept
	{
		return p[idx] == 0u;
	}

	struct DefensiveMoves
	{
		private:
			ShortVector<Location, 24> list;
			bool not_initialized = true;
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
	MoveGenerator::MoveGenerator(GameConfig gameConfig, PatternCalculator &calc) :
			game_config(gameConfig),
			pattern_calculator(calc),
			moves(gameConfig.rows, gameConfig.cols),
			m_total_time("total_generator_time   "),
			m_defensive_moves("get_defensive_moves    "),
			m_draw_in_1("try_draw_in_1          "),
			m_win_in_1("try_win_in_1           "),
			m_loss_in_2("defend_loss_in_2       "),
			m_win_in_3("try_win_in_3           "),
			m_loss_in_4("defend_loss_in_4       "),
			m_win_in_5("try_win_in_5           "),
			m_loss_in_6("defend_loss_in_6       "),
			m_win_in_7("try_win_in_7           "),
			m_forbidden_moves("mark_forbidden_moves   "),
			m_mark_neighborhood("mark_neighborhood      "),
			m_remaining_moves("create_remaining_moves ")
	{
		temporary_list.reserve(64);
		forbidden_moves_cache.reserve(32);
	}
	MoveGenerator::~MoveGenerator()
	{
//		std::cout << m_total_time.toString() << '\n';
//		std::cout << m_defensive_moves.toString() << '\n';
//		std::cout << m_draw_in_1.toString() << '\n';
//		std::cout << m_win_in_1.toString() << '\n';
//		std::cout << m_loss_in_2.toString() << '\n';
//		std::cout << m_win_in_3.toString() << '\n';
//		std::cout << m_loss_in_4.toString() << '\n';
//		std::cout << m_win_in_5.toString() << '\n';
//		std::cout << m_loss_in_6.toString() << '\n';
//		std::cout << m_win_in_7.toString() << '\n';
//		std::cout << m_forbidden_moves.toString() << '\n';
//		std::cout << m_mark_neighborhood.toString() << '\n';
//		std::cout << m_remaining_moves.toString() << '\n';
	}
	Score MoveGenerator::generate(ActionList &actions, MoveGeneratorMode mode)
	{
		TimerGuard tg(m_total_time);
		assert(this->actions == nullptr);
		assert(actions.isEmpty());

		this->actions = &actions;
		moves.fill(false);
		forbidden_moves_cache.clear();

		const int distance_to_draw = game_config.draw_after - pattern_calculator.getCurrentDepth();

		Result result;
		if (result.must_continue and distance_to_draw >= 1)
			result = try_win_in_1();
		if (result.must_continue and distance_to_draw == 1)
			result = try_draw_in_1();
		if (mode == MoveGeneratorMode::THREATS or mode == MoveGeneratorMode::OPTIMAL)
		{
			if (result.must_continue and distance_to_draw >= 2)
				result = defend_loss_in_2();
			if (result.must_continue and distance_to_draw >= 3)
				result = try_win_in_3();
			if (result.must_continue and distance_to_draw >= 4)
				result = defend_loss_in_4();
			if (result.must_continue and distance_to_draw >= 5)
				result = try_win_in_5();
			if (result.must_continue and distance_to_draw >= 6)
				result = defend_loss_in_6();
//			if (result.must_continue and distance_to_draw >= 7)
//				result = try_win_in_7();
			if (result.must_continue and distance_to_draw >= 3)
				add_own_half_open_fours();
		}
		if (result.must_continue and mode >= MoveGeneratorMode::OPTIMAL)
		{
			if (mode == MoveGeneratorMode::OPTIMAL)
			{
				if (distance_to_draw >= 6)
				{
					add_moves<EXCLUDE_DUPLICATE>(get_opponent_threats(ThreatType::FORK_3x3), Score(3));
					add_moves<EXCLUDE_DUPLICATE>(get_opponent_threats(ThreatType::OPEN_3), Score(2));
				}
				if (distance_to_draw >= 5)
				{
					add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::FORK_3x3), Score(13));
					add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::OPEN_3), Score(1));
				}
				if (distance_to_draw >= 3)
					add_moves<EXCLUDE_DUPLICATE>(get_opponent_threats(ThreatType::HALF_OPEN_4), Score(4));
			}

			const BitMask2D<uint32_t, 32> mask = (mode <= MoveGeneratorMode::REDUCED) ? mark_neighborhood() : pattern_calculator.getLegalMovesMask();
			create_remaining_moves(mask);
		}
		if (is_anything_forbidden_for(get_own_sign()))
			mark_forbidden_moves();

		actions.is_fully_expanded = actions.must_defend or mode >= MoveGeneratorMode::OPTIMAL;

		this->actions = nullptr;
		return result.score;
	}
	/*
	 * private
	 */
	template<MoveGenerator::AddMode Mode>
	void MoveGenerator::add_move(Location loc, Score s) noexcept
	{
		if (moves.at(loc.row, loc.col) == true)
		{
			if (Mode == OVERRIDE_DUPLICATE)
			{
				for (auto iter = actions->begin(); iter < actions->end(); iter++)
					if (iter->move.location() == loc)
					{
						iter->score = s;
						return;
					}
				assert(false); // the move must be contained in the action list
			}
		}
		else
		{
			actions->add(Move(get_own_sign(), loc), s);
			moves.at(loc.row, loc.col) = true;
		}
	}
	template<MoveGenerator::AddMode Mode>
	void MoveGenerator::add_moves(const LocationList &moves, Score s) noexcept
	{
		for (auto iter = moves.begin(); iter < moves.end(); iter++)
			add_move<Mode>(*iter, s);
	}
	template<MoveGenerator::AddMode Mode, typename T>
	void MoveGenerator::add_moves(const T begin, const T end, Score s) noexcept
	{
		for (auto iter = begin; iter < end; iter++)
			add_move<Mode>(*iter, s);
	}
	ShortVector<Location, 6> MoveGenerator::get_defensive_moves(Location move, Direction dir)
	{
//		TimerGuard tg(m_defensive_moves);
		ShortVector<Location, 6> result = pattern_calculator.getDefensiveMoves(get_own_sign(), move.row, move.col, dir);
		if (is_anything_forbidden_for(get_own_sign()))
		{ // forbidden moves require special handling
			int i = 0;
			while (i < result.size())
			{
				if (is_forbidden(get_own_sign(), result[i]))
				{ // if a defensive move is forbidden, we assign appropriate loss score to that move and exclude it from the list
					add_move<OVERRIDE_DUPLICATE>(result[i], Score::loss_in(1));
					result.remove(i);
				}
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
	MoveGenerator::Result MoveGenerator::try_draw_in_1()
	{
//		TimerGuard tg(m_draw_in_1);
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);
		assert(pattern_calculator.getCurrentDepth() + 1 >= game_config.draw_after);

		actions->baseline_score = Score::draw_in(1);
		if (is_anything_forbidden_for(get_own_sign()))
		{ // only if there can be any forbidden move, so in renju and for cross (black) player
			bool found_at_least_one = false;
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
					if (pattern_calculator.signAt(row, col) == Sign::NONE)
					{
						const Location loc(row, col);
						const ThreatType threat = get_own_threat_at(row, col);
						switch (threat)
						{
							default: // no forbidden threat, add this move and exit
								add_move<EXCLUDE_DUPLICATE>(loc, Score::draw_in(1));
								found_at_least_one = true;
								break;
							case ThreatType::FORK_3x3: // possibly forbidden
								if (is_forbidden(get_own_sign(), loc))
									add_move<EXCLUDE_DUPLICATE>(loc, Score::loss_in(1));
								else
								{
									add_move<EXCLUDE_DUPLICATE>(loc, Score::draw_in(1));
									found_at_least_one = true;
								}
								break;
							case ThreatType::FORK_4x4: // obviously forbidden
							case ThreatType::OVERLINE: // obviously forbidden
								add_move<EXCLUDE_DUPLICATE>(loc, Score::loss_in(1));
								break;
						}
					}

			if (found_at_least_one)
				return Result::canStopNow(Score::draw_in(1));
			else
				return Result::canStopNow(Score::loss_in(1)); // could not find any non-forbidden move
		}
		else
		{ // if there are no forbidden moves, mark all legal moves as draw
			create_remaining_moves(pattern_calculator.getLegalMovesMask(), Score::draw_in(1));
			return Result::canStopNow(Score::draw_in(1)); // the game ends up with a draw even if there is no move to play
		}
	}
	MoveGenerator::Result MoveGenerator::try_win_in_1()
	{
//		TimerGuard tg(m_win_in_1);
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		const LocationList &own_fives = get_own_threats(ThreatType::FIVE);
		if (own_fives.size() > 0)
		{ // not often (approximately 1 in 600 positions, in renju 1 in 200)
			actions->has_initiative = true;
			add_moves<EXCLUDE_DUPLICATE>(own_fives, Score::win_in(1));
			return Result::canStopNow(Score::win_in(1));
		}
		return Result::mustContinue();
	}
	MoveGenerator::Result MoveGenerator::defend_loss_in_2()
	{
//		TimerGuard tg(m_loss_in_2);
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		if (get_opponent_threats(ThreatType::FIVE).size() == 0)
			return Result::mustContinue();

		const LocationList &opponent_fives = get_opponent_threats(ThreatType::FIVE);
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
			assert(is_forbidden(get_own_sign(), *move) == false); // forbidden moves should have been excluded earlier
			Score response_score;
			switch (get_own_threat_at(move->row, move->col))
			{
				case ThreatType::FORK_3x3:
				{
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
					if (solution.isProven())
						response_score = solution; // found a surely winning 4x3 fork
					else
						response_score = Score(15); // assign some prior score for ordering purposes
					break;
				}
				case ThreatType::FORK_4x4:
				{ // rare (approximately 1 in 3000 positions)
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
						response_score = Score(14); // assign some prior score for ordering purposes
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
	MoveGenerator::Result MoveGenerator::try_win_in_3()
	{
//		TimerGuard tg(m_win_in_3);
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		int threat_count = 0;
		if (is_anything_forbidden_for(get_own_sign()))
		{ // in renju there might be an open four hidden inside a legal 3x3 fork
		  // it is very rare (approximately 1 in 500.000 positions) but we must check this in order to have correct results

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const LocationList &opp_fork_3x3 = get_copy_of(get_own_threats(ThreatType::FORK_3x3));
			for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
			{
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
				if (group.contains(PatternType::OPEN_4) and not is_forbidden(get_own_sign(), *move))
				{
					threat_count++;
					add_move<EXCLUDE_DUPLICATE>(*move, Score::win_in(3));
				}
			}
		}

		const LocationList &own_open_four = get_own_threats(ThreatType::OPEN_4);
		add_moves<EXCLUDE_DUPLICATE>(own_open_four, Score::win_in(3));
		threat_count += own_open_four.size();

		const LocationList &own_fork_4x4 = get_own_threats(ThreatType::FORK_4x4);
		if (own_fork_4x4.size() > 0 and not is_anything_forbidden_for(get_own_sign()))
		{ // rare (approximately 1 in 1700 positions)
			threat_count += own_fork_4x4.size();
			add_moves<EXCLUDE_DUPLICATE>(own_fork_4x4, Score::win_in(3));
		}

		if (is_anything_forbidden_for(get_opponent_sign()))
		{ // in renju, we might be able to create a threat of five where the opponent will have a forbidden move - only for circle (white) player
		  // it is quite frequent (approximately 1 in 300 positions) and we can successfully solve ~98% of such positions

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const LocationList &own_half_open_four = get_copy_of(get_own_threats(ThreatType::HALF_OPEN_4));
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
					threat_count++;
					return Result::canStopNow(Score::win_in(3));
				}
			}
			// there are also half-open fours hidden inside legal 3x3 forks, but they are less than 0.5% of all half-open fours, so we don't check it here
		}
		if (threat_count > 0)
		{
			actions->has_initiative = true;
			return Result::canStopNow(Score::win_in(3));
		}
		else
			return Result::mustContinue();
	}
	MoveGenerator::Result MoveGenerator::defend_loss_in_4()
	{
//		TimerGuard tg(m_loss_in_4);
		assert(actions->size() == 0);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		const bool has_any_four = pattern_calculator.getThreatHistogram(get_own_sign()).hasAnyFour();
		actions->baseline_score = Score::loss_in(4); // will be reverted at the end if there are no threats

		if (game_config.rules != GameRules::RENJU)
		{
			// in order to find a minimal number of defensive moves we must calculate an intersection of defensive moves to all individual threats
			// if there are any moves that occur as a refutation to all threats we can make them
			// otherwise, if there is no move that can defend against all threats it is a loss in 4 (provided that we can't make any own fours)
			DefensiveMoves defensive_moves;

			const LocationList &opp_open_four = get_opponent_threats(ThreatType::OPEN_4);
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

			const LocationList &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
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
				// we would need to check all combinations and pick the one that has the largest intersection with defensive moves
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
				const LocationList &opp_open_four = get_copy_of(get_opponent_threats(ThreatType::OPEN_4));
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
				const LocationList &opp_fork_3x3 = get_copy_of(get_opponent_threats(ThreatType::FORK_3x3));
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
				const LocationList &opp_fork_4x4 = get_copy_of(get_opponent_threats(ThreatType::FORK_4x4));
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
		{
			actions->has_initiative = has_any_four;
			const Score best_score = add_own_4x3_forks();
			add_own_half_open_fours();
			if (best_score.isWin())
				return Result::canStopNow(best_score);
			return Result::canStopNow();
		}
		else
			actions->baseline_score = Score();
		return Result::mustContinue();
	}
	MoveGenerator::Result MoveGenerator::try_win_in_5()
	{
//		TimerGuard tg(m_win_in_5);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		Score best_score = add_own_4x3_forks();

		if (not is_anything_forbidden_for(get_own_sign()))
		{ // in renju, for cross (black) player it is possible that in 3x3 fork some open 3 can't be converted to a four because they will be forbidden for some reason
		  // but it is too complicated to solve statically, so we check only for circle (white) player
			if (number_of_available_fours_for(get_opponent_sign()) == 0)
			{ // opponent has no four to make
				const LocationList &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
				if (own_fork_3x3.size() > 0)
				{ // it happens quite often (approximately 1 in 150 positions)
					add_moves<EXCLUDE_DUPLICATE>(own_fork_3x3, Score::win_in(5));
					best_score = std::max(best_score, Score::win_in(5));
				}
			}
		}

		if (best_score.isWin())
		{
			actions->has_initiative = true;
			return Result::canStopNow(best_score);
		}
		else
			return Result::mustContinue();
	}
	MoveGenerator::Result MoveGenerator::defend_loss_in_6()
	{
//		TimerGuard tg(m_loss_in_6);
		assert(actions->baseline_score == Score());
		if (number_of_available_fours_for(get_own_sign()) > 0)
			return Result::mustContinue();

		const int fork_4x3_count = get_opponent_threats(ThreatType::FORK_4x3).size();
		const int fork_3x3_count = get_opponent_threats(ThreatType::FORK_3x3).size();
		if (fork_4x3_count > 0 or fork_3x3_count > 0)
		{ // if the opponent has at least one fork the position is a defensive one
			actions->must_defend = true;
			actions->baseline_score = Score::loss_in(6);
		}

		if (fork_4x3_count > 0)
		{ // starting from 4x3 forks as they are easier
			const LocationList &opp_fork_4x3 = get_opponent_threats(ThreatType::FORK_4x3);
			for (auto move = opp_fork_4x3.begin(); move < opp_fork_4x3.end(); move++)
			{
				// the analysis for 4x3 forks is very complex so we don't try to solve them exactly
				// instead, rather than trying to create a minimal list of moves we just exclude those of them that are obviously losing
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);

				for (Direction dir = 0; dir < 4; dir++)
					if (group[dir] == PatternType::OPEN_3)
					{ // we start from adding all defensive moves to the open 3 forming the fork
						const ShortVector<Location, 6> tmp = get_defensive_moves(*move, dir);
						add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end(), Score(0));
					}

				// then we find defensive moves to the half open 4 in the fork
				const Direction direction = group.findDirectionOf(PatternType::HALF_OPEN_4);
				const ShortVector<Location, 6> half_open_4_defensive_moves = get_defensive_moves(*move, direction);
				add_moves<EXCLUDE_DUPLICATE>(half_open_4_defensive_moves.begin(), half_open_4_defensive_moves.end(), Score(0));

				// we must also add moves around the defensive moves to half open 4
				// if they can form at least half open 3 it might be possible to regain initiative after playing them
				for (auto iter = half_open_4_defensive_moves.begin(); iter < half_open_4_defensive_moves.end(); iter++)
				{ // we check every direction around the defensive move
					for (Direction dir = 0; dir < 4; dir++)
					{
						const ReducedPattern raw_pattern = pattern_calculator.getReducedPatternAt(iter->row, iter->col, dir);
						for (int i = -4; i <= 4; i++)
							if (is_spot_empty(i, raw_pattern))
							{
								const Location tmp = shiftInDirection(dir, i, *iter);
								const PatternType pt = pattern_calculator.getPatternTypeAt(get_own_sign(), tmp.row, tmp.col, dir);
								if ((pt > PatternType::NONE) or pattern_calculator.isHalfOpenThreeAt(tmp.row, tmp.col, dir, get_own_sign()))
									add_move<EXCLUDE_DUPLICATE>(tmp);
							}
					}
				}
			}
		}

		if (fork_3x3_count > 0)
		{
			const LocationList &opp_fork_3x3 = get_opponent_threats(ThreatType::FORK_3x3);
			for (auto move = opp_fork_3x3.begin(); move < opp_fork_3x3.end(); move++)
			{
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
				for (Direction dir = 0; dir < 4; dir++)
					if (group[dir] == PatternType::OPEN_3)
					{ // we start from adding all defensive moves to the open 3 forming the fork
						const ShortVector<Location, 6> tmp = get_defensive_moves(*move, dir);
						add_moves<EXCLUDE_DUPLICATE>(tmp.begin(), tmp.end(), Score(0));
					}

				// then add our remaining threats
				add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::FORK_3x3), Score(13));
				add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::OPEN_3), Score(1));
				// half open fours will be added at the end

				// now we must identify all half open threes on board
				const BitMask2D<uint32_t, 32> mask = mark_star_like_pattern_for(get_own_sign()); // at first create a mask of all relevant moves to be checked
				for (int row = 0; row < game_config.rows; row++)
				{
					uint32_t tmp = mask[row] & (~moves[row]); // exclude moves that have already been added (stored in 'moves')
					for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
						if (tmp & 1)
						{
							for (Direction dir = 0; dir < 4; dir++)
								if (pattern_calculator.isHalfOpenThreeAt(row, col, dir, get_own_sign()))
								{
									add_move<EXCLUDE_DUPLICATE>(Location(row, col), Score(1));
									break;
								}
						}
				}
			}
		}

		if (actions->must_defend)
		{
			add_own_half_open_fours();
			return Result::canStopNow();
		}
		else
			return Result::mustContinue();
	}
	MoveGenerator::Result MoveGenerator::try_win_in_7()
	{
//		TimerGuard tg(m_win_in_7);
		assert(actions->baseline_score == Score());
		assert(actions->must_defend == false && actions->has_initiative == false);

		static ScopedCounter total_calls("total calls");
		static ScopedCounter total_hits4("total hits4");
		static ScopedCounter total_hits3("total hits3");
		static ScopedCounter total_hits2("total hits2");
		static ScopedCounter total_hits1("total hits1");

		// in renju, for cross (black) player it is possible that in 3x3 fork some open 3 can't be converted to a four because they will be forbidden for some reason
		// but it is too complicated to solve statically
		if (game_config.rules == GameRules::RENJU)
			return Result::mustContinue();

		total_calls.increment();

		if (get_opponent_threats(ThreatType::FORK_4x3).size() > 0 or get_opponent_threats(ThreatType::FORK_3x3).size() > 0)
			return Result::mustContinue();
		total_hits1.increment();

		const LocationList &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
		if (own_fork_3x3.size() == 0)
			return Result::mustContinue();
		total_hits2.increment();

		const LocationList &opp_half_open_4 = get_opponent_threats(ThreatType::HALF_OPEN_4);

		// there are several conditions to be met, checking them from the least costly one
		DefensiveMoves defensive_moves;
		for (auto move = opp_half_open_4.begin(); move < opp_half_open_4.end(); move++)
		{
			const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
			const Direction direction = group.findDirectionOf(PatternType::HALF_OPEN_4);
			const ShortVector<Location, 6> tmp = get_defensive_moves(*move, direction);

			defensive_moves.get_intersection_with(tmp);
			if (defensive_moves.is_empty())
				return Result::mustContinue(); // there are several not related half-open fours
		}
		total_hits3.increment();

		// then check if there are no half-open threes
		for (auto move = opp_half_open_4.begin(); move < opp_half_open_4.end(); move++)
			for (Direction dir = 0; dir < 4; dir++)
				if (pattern_calculator.isHalfOpenThreeAt(move->row, move->col, dir, get_opponent_sign()))
					return Result::mustContinue();
		total_hits4.increment();

//			pattern_calculator.print();
//			pattern_calculator.printAllThreats();
//			exit(0);

		bool found_free_fork = false;
//			for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
//			{
//			}
//			total_hits3.increment();
		if (found_free_fork)
			return Result::canStopNow(Score::win_in(7));
		else
			return Result::mustContinue();
	}
	Score MoveGenerator::add_own_4x3_forks()
	{
		Score result;
		const LocationList &own_fork_4x3 = get_own_threats(ThreatType::FORK_4x3);
		for (auto iter = own_fork_4x3.begin(); iter < own_fork_4x3.end(); iter++)
		{
			const Score solution = try_solve_own_fork_4x3(*iter);
			add_move<OVERRIDE_DUPLICATE>(*iter, solution); // the move might have been added earlier as defensive move, so we must override its score
			if (solution.isProven())
				result = std::max(result, solution);
		}
		return result;
	}
	void MoveGenerator::add_own_half_open_fours()
	{
		constexpr Score prior_move_score(14);

		int hidden_count = 0;
		if (is_anything_forbidden_for(get_own_sign()))
		{ // in renju rule there might be a half-open four hidden inside a legal 3x3 fork - only for cross (black) player
		  // it is rare (approximately 1 in 60.000 positions) but cheap to check

			// we must copy as the check for forbidden moves may reorder the elements in the original list
			const LocationList &own_fork_3x3 = get_copy_of(get_own_threats(ThreatType::FORK_3x3));
			for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
			{
				const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
				if (group.contains(PatternType::HALF_OPEN_4) and not is_forbidden(get_own_sign(), *move))
				{
					add_move<EXCLUDE_DUPLICATE>(*move, prior_move_score);
					hidden_count++;
				}
			}
		}

		// in theory we can exclude half-open fours where the opponent can create a winning threat in response
		// they are rare (less than 0.5% of all half-open fours) and the check is very costly so we don't do this
//		const std::vector<Location> &own_half_open_fours = get_own_threats(ThreatType::HALF_OPEN_4);
//		for (auto move = own_half_open_fours.begin(); move < own_half_open_fours.end(); move++)
//		{
//			const DirectionGroup<PatternType> group = pattern_calculator.getPatternTypeAt(get_own_sign(), move->row, move->col);
//			const Direction dir = group.findDirectionOf(PatternType::HALF_OPEN_4);
//			ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_opponent_sign(), move->row, move->col, dir);
//			defensive_moves.remove(*move); // excluding move at the spot that we occupied to create this threat
//
//			ThreatType best_opponent_threat = ThreatType::NONE;
//			for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
//				best_opponent_threat = std::max(best_opponent_threat, get_opponent_threat_at(iter->row, iter->col));
//
//			switch (best_opponent_threat)
//			{
//				default:
//				{
//					hidden_count++;
//					add_move<EXCLUDE_DUPLICATE>(*move);
//					break;
//				}
//				case ThreatType::FORK_4x4:
//				case ThreatType::OPEN_4:
//				case ThreatType::FIVE:
//					break;
//			}
//		}
//		return hidden_count;

		add_moves<EXCLUDE_DUPLICATE>(get_own_threats(ThreatType::HALF_OPEN_4), prior_move_score); // half-open four is a threat of win in 1
		const int total_count = hidden_count + get_own_threats(ThreatType::HALF_OPEN_4).size();
		if (total_count > 0)
			actions->has_initiative = true;
	}
	Score MoveGenerator::try_solve_own_fork_4x3(Location move)
	{
		constexpr Score prior_move_score(15);

		assert(get_own_threat_at(move.row, move.col) == ThreatType::FORK_4x3);
		if (is_anything_forbidden_for(get_own_sign())) // unfortunately it is very often (approximately i in 30 positions)
			return prior_move_score; // there is a possibility that inside the three there will later be a forbidden move, it's too complicated to check statically

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
				return prior_move_score; // too complicated to check statically
			case ThreatType::FORK_4x4: // rare (approximately 1 in 4500 positions)
			case ThreatType::OPEN_4:
				return Score::loss_in(4); // opponent will create a 'win in 3' type of threat
			case ThreatType::FIVE: // actually this should never happen, as if the opponent has any five it should have been handled earlier in 'defend_loss_in_2()'
			case ThreatType::OVERLINE:
				return Score::loss_in(2); // opponent will create winning threat in response
		}
	}
	void MoveGenerator::mark_forbidden_moves()
	{
//		TimerGuard tg(m_forbidden_moves);
		assert(get_own_sign() == Sign::CROSS);
		assert(get_own_threats(ThreatType::FIVE).size() == 0); // win in 1 threats should have been checked earlier
		// because of the above assumptions we can now just add overlines and 4x4 forks, as we know that they don't contain a winning move
		add_moves<OVERRIDE_DUPLICATE>(get_own_threats(ThreatType::OVERLINE), Score::loss_in(1));
		add_moves<OVERRIDE_DUPLICATE>(get_own_threats(ThreatType::FORK_4x4), Score::loss_in(1));

		// we only need to check 3x3 forks because they are non-trivial
		const LocationList &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
		for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
			if (is_forbidden(Sign::CROSS, *move))
				add_move<OVERRIDE_DUPLICATE>(*move, Score::loss_in(1));
	}
	BitMask2D<uint32_t, 32> MoveGenerator::mark_neighborhood()
	{
//		TimerGuard tg(m_mark_neighborhood);
		/*
		 * Mask of the following pattern:
		 * ! _ _ ! _ _ !  -> 1001001 = 73
		 * _ ! ! ! ! ! _  -> 0111110 = 62
		 * _ ! ! ! ! ! _  -> 0111110 = 62
		 * ! ! ! _ ! ! !  -> 1110111 = 119
		 * _ ! ! ! ! ! _  -> 0111110 = 62
		 * _ ! ! ! ! ! _  -> 0111110 = 62
		 * ! _ _ ! _ _ !  -> 1001001 = 73
		 */
#ifdef __AVX2__
		static const __m256i masks = _mm256_slli_epi32(_mm256_setr_epi32(73u, 62u, 62u, 119u, 62u, 62u, 73u, 0u), 25u);
#elif defined(__SSE2__)
		static const __m128i masks1 = _mm_slli_epi32(_mm_setr_epi32(73u, 62u, 62u, 119u), 25u);
		static const __m128i masks2 = _mm_slli_epi32(_mm_setr_epi32(62u, 62u, 73u, 0u), 25u);
#else
		// masks are shifted maximally to the left
		static const uint32_t masks[7] =
		{	73u << 25u, 62u << 25u, 62u << 25u, 119u << 25u, 62u << 25u, 62u << 25u, 73u << 25u, 0u};
#endif

		constexpr int row_offset = 3;
		const BitMask2D<uint32_t> &legal_moves = pattern_calculator.getLegalMovesMask();
		BitMask2D<uint32_t, 32> result(row_offset + game_config.rows + row_offset + 1, game_config.cols);

		for (int row = 0; row < game_config.rows; row++)
		{
			uint32_t tmp = legal_moves[row];
			for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
				if ((tmp & 1) == 0)
				{ // the spot is not legal -> it is occupied
#ifdef __AVX2__
					const __m256i shift = _mm256_set1_epi32(28 - col);
					const __m256i x = _mm256_loadu_si256((__m256i*) (result.data() + row));
					_mm256_storeu_si256((__m256i*) (result.data() + row), _mm256_or_si256(x, _mm256_srlv_epi32(masks, shift)));
#elif defined(__SSE2__)
					const __m128i shift = _mm_set_epi32(0, 0, 0, 28 - col);
					const __m128i x1 = _mm_loadu_si128((__m128i*) (result.data() + row));
					const __m128i x2 = _mm_loadu_si128((__m128i*) (result.data() + row + 4));
					_mm_storeu_si128((__m128i*) (result.data() + row), _mm_or_si128(x1, _mm_srl_epi32(masks1, shift)));
					_mm_storeu_si128((__m128i*) (result.data() + row + 4), _mm_or_si128(x2, _mm_srl_epi32(masks2, shift)));
#else
					const int shift = 28 - col; // now calculate how much the masks need to be shifted to the right to match desired position
					for (int i = 0; i < 7; i++)
						result[row + i] |= (result[i] >> shift);// now just apply patterns to the board
#endif
				}
		}
		if (pattern_calculator.getCurrentDepth() == 0) // no moves were made on board
			result.at(row_offset + game_config.rows / 2, game_config.cols / 2) = true; // mark single move at the center

		// result[row_offset + i] is a mask of all neighboring moves to all stones already on board
		// but they might be illegal, so it is masked with legal_moves[i]
		// and it is assigned back to the result[i] because this array will be used for adding all moves that are left
		for (int i = 0; i < game_config.rows; i++)
			result[i] = result[row_offset + i] & legal_moves[i];
		return result;
	}
	BitMask2D<uint32_t, 32> MoveGenerator::mark_star_like_pattern_for(Sign s)
	{
		assert(s == Sign::CROSS || s == Sign::CIRCLE);
		/*
		 * Mask of the following pattern:
		 * ! _ _ ! _ _ !  -> 1001001 = 73
		 * _ ! _ ! _ ! _  -> 0101010 = 42
		 * _ _ ! ! ! _ _  -> 0011100 = 28
		 * ! ! ! _ ! ! !  -> 1110111 = 119
		 * _ _ ! ! ! _ _  -> 0011100 = 28
		 * _ ! _ ! _ ! _  -> 0101010 = 42
		 * ! _ _ ! _ _ !  -> 1001001 = 73
		 */
#ifdef __AVX2__
		static const __m256i masks = _mm256_slli_epi32(_mm256_setr_epi32(73u, 42u, 28u, 119u, 28u, 42u, 73u, 0u), 25u);
#elif defined(__SSE2__)
		static const __m128i masks1 = _mm_slli_epi32(_mm_setr_epi32(73u, 42u, 28u, 119u), 25u);
		static const __m128i masks2 = _mm_slli_epi32(_mm_setr_epi32(28u, 42u, 73u, 0u), 25u);
#else
		// masks are shifted maximally to the left
		static const uint32_t masks[7] =
		{	73u << 25u, 42u << 25u, 28u << 25u, 119u << 25u, 28u << 25u, 42u << 25u, 73u << 25u, 0u};
#endif

		constexpr int row_offset = 3;
		const BitMask2D<uint32_t> &legal_moves = pattern_calculator.getLegalMovesMask();
		BitMask2D<uint32_t, 32> result(row_offset + game_config.rows + row_offset + 1, game_config.cols);

		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (pattern_calculator.signAt(row, col) == s)
				{ // the spot is not legal -> it is occupied
#ifdef __AVX2__
					const __m256i shift = _mm256_set1_epi32(28 - col);
					const __m256i x = _mm256_loadu_si256((__m256i*) (result.data() + row));
					_mm256_storeu_si256((__m256i*) (result.data() + row), _mm256_or_si256(x, _mm256_srlv_epi32(masks, shift)));
#elif defined(__SSE2__)
					const __m128i shift = _mm_set_epi32(0, 0, 0, 28 - col);
					const __m128i x1 = _mm_loadu_si128((__m128i*) (result.data() + row));
					const __m128i x2 = _mm_loadu_si128((__m128i*) (result.data() + row + 4));
					_mm_storeu_si128((__m128i*) (result.data() + row), _mm_or_si128(x1, _mm_srl_epi32(masks1, shift)));
					_mm_storeu_si128((__m128i*) (result.data() + row + 4), _mm_or_si128(x2, _mm_srl_epi32(masks2, shift)));
#else
					const int shift = 28 - col; // now calculate how much the masks need to be shifted to the right to match desired position
					for (int i = 0; i < 7; i++)
						result[row + i] |= (result[i] >> shift);// now just apply patterns to the board
#endif
				}
		// result[row_offset + i] is a mask of all neighboring moves to all stones already on board
		// but they might be illegal, so it is masked with legal_moves[i]
		// and it is assigned back to the result[i]
		for (int i = 0; i < game_config.rows; i++)
			result[i] = result[row_offset + i] & legal_moves[i];
		return result;
	}
	void MoveGenerator::create_remaining_moves(const BitMask2D<uint32_t, 32> &mask, Score s)
	{
//		TimerGuard tg(m_remaining_moves);
		for (int row = 0; row < game_config.rows; row++)
		{
			uint32_t tmp = mask[row] & (~moves[row]); // exclude moves that have already been added (stored in 'moves')
			for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
				actions->add(Move(get_own_sign(), row, col), s, tmp & 1);
			moves[row] |= mask[row]; // update bitmask of added moves
		}
	}

	Sign MoveGenerator::get_own_sign() const noexcept
	{
		return pattern_calculator.getSignToMove();
	}
	Sign MoveGenerator::get_opponent_sign() const noexcept
	{
		return invertSign(get_own_sign());
	}
	ThreatType MoveGenerator::get_own_threat_at(int row, int col) const noexcept
	{
		return pattern_calculator.getThreatAt(get_own_sign(), row, col);
	}
	ThreatType MoveGenerator::get_opponent_threat_at(int row, int col) const noexcept
	{
		return pattern_calculator.getThreatAt(get_opponent_sign(), row, col);
	}
	const LocationList& MoveGenerator::get_own_threats(ThreatType tt) const noexcept
	{
		return pattern_calculator.getThreatHistogram(get_own_sign()).get(tt);
	}
	const LocationList& MoveGenerator::get_opponent_threats(ThreatType tt) const noexcept
	{
		return pattern_calculator.getThreatHistogram(get_opponent_sign()).get(tt);
	}
	bool MoveGenerator::is_anything_forbidden_for(Sign sign) const noexcept
	{
		return (game_config.rules == GameRules::RENJU) and (sign == Sign::CROSS);
	}
	bool MoveGenerator::is_forbidden(Sign sign, Location loc) const noexcept
	{
		if (is_anything_forbidden_for(sign))
		{ // caching forbidden moves is not that much useful, it has about 9% hit rate
		  // and saves ~0.56% of pattern recalculations
			for (size_t i = 0; i < forbidden_moves_cache.size(); i++)
				if (forbidden_moves_cache[i].first == loc)
					return forbidden_moves_cache[i].second;
			const bool result = pattern_calculator.isForbidden(sign, loc.row, loc.col);
			forbidden_moves_cache.push_back(std::pair<Location, bool>(loc, result));
			return result;
		}
		return false;
	}
	const LocationList& MoveGenerator::get_copy_of(const LocationList &list)
	{
		temporary_list = list;
		return temporary_list;
	}
	bool MoveGenerator::is_move_valid(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}
	bool MoveGenerator::is_possible(Score s) const noexcept
	{
		if (s == Score::draw_in(1))
			return pattern_calculator.getCurrentDepth() + 1 >= game_config.draw_after;
		if (s == Score::win_in(1))
			return get_own_threats(ThreatType::FIVE).size() > 0;
		if (s == Score::loss_in(2))
			return get_opponent_threats(ThreatType::FIVE).size() > 0;
		return false;
	}
	int MoveGenerator::number_of_available_fours_for(Sign sign) const noexcept
	{
		const int open_4_count = pattern_calculator.getThreatHistogram(sign).get(ThreatType::OPEN_4).size();
		const int fork_4x4_count = is_anything_forbidden_for(sign) ? 0 : pattern_calculator.getThreatHistogram(sign).get(ThreatType::FORK_4x4).size();
		const int fork_4x3_count = pattern_calculator.getThreatHistogram(sign).get(ThreatType::FORK_4x3).size();
		const int half_open_4_count = pattern_calculator.getThreatHistogram(sign).get(ThreatType::HALF_OPEN_4).size();

		return open_4_count + fork_4x4_count + fork_4x3_count + half_open_4_count;
	}

} /* namespace ag */
