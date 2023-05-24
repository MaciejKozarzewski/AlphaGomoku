/*
 * MoveGenerator.cpp
 *
 *  Created on: Oct 2, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/MoveGenerator.hpp>
#include <alphagomoku/ab_search/Score.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <cassert>
#include <x86intrin.h>

namespace
{
	using namespace ag;
	bool contains(const std::vector<Location> &list, Move m)
	{
		for (auto iter = list.begin(); iter < list.end(); iter++)
			if (iter->row == m.row and iter->col == m.col)
				return true;
		return false;
	}
}

namespace ag
{
	namespace obsolete
		{
	MoveGenerator::MoveGenerator(GameConfig gameConfig, PatternCalculator &calc, ActionList &actionList, MoveGeneratorMode mode) :
			CommonBase(gameConfig, calc),
			moves(gameConfig.rows, gameConfig.cols),
			actions(actionList),
			mode(mode)
	{
	}
	void MoveGenerator::setExternalStorage(std::vector<Location> &tmp) noexcept
	{
		set_external_temporary_list(tmp);
	}
	void MoveGenerator::set(Move hashMove, const ShortVector<Move, 4> &killerMoves) noexcept
	{
		hash_move = hashMove;
		killer_moves = killerMoves;
	}
	void MoveGenerator::generate()
	{
		if (generator_stage == NOT_STARTED)
		{
			assert(actions.size() == 0);
			switch (mode)
			{
				case MoveGeneratorMode::LEGAL:
					moves = pattern_calculator.getLegalMovesMask();
					generator_stage = ALL_GENERATED;
					break;
				case MoveGeneratorMode::REDUCED:
					generator_stage = REMAINING_MOVES;
					break;
				case MoveGeneratorMode::NORMAL:
				case MoveGeneratorMode::THREATS:
				default:
				{
					generator_stage = HASH_MOVE;
					break;
				}
			}
		}

		if (generator_stage == HASH_MOVE)
		{
			generator_stage = FORCING_THREATS;
			if (is_move_valid(hash_move))
			{
				add_move(hash_move);
				return;
			}
		}

		if (generator_stage == FORCING_THREATS)
		{
			generator_stage = ALL_GENERATED;
			if (try_win_in_1())
				return;
			if (defend_loss_in_2())
				return;
			if (try_win_in_3())
				return;
			if (defend_loss_in_4())
				return;
			if (try_win_in_5())
				return;
			if (defend_loss_in_6())
				return;
			generator_stage = KILLER_MOVES;
		}

		if (generator_stage == KILLER_MOVES)
		{
			generator_stage = WEAKER_THREATS;
			const int initial_action_count = actions.size();
			for (auto iter = killer_moves.begin(); iter < killer_moves.end(); iter++)
			{
				const bool tmp1 = (mode == MoveGeneratorMode::THREATS) ? contains(get_own_threats(ThreatType::HALF_OPEN_4), *iter) : true;
				const bool tmp2 = (mode == MoveGeneratorMode::THREATS) ? contains(get_own_threats(ThreatType::OPEN_3), *iter) : true;
				if (is_move_valid(*iter) and (tmp1 or tmp2))
					add_move(*iter);
			}
			if (actions.size() > initial_action_count)
				return; // we have generated some new moves
		}

		if (generator_stage == WEAKER_THREATS)
		{
			const int initial_action_count = actions.size();
			add_moves(get_own_threats(ThreatType::HALF_OPEN_4));
			if (mode == MoveGeneratorMode::THREATS)
				generator_stage = ALL_GENERATED;
			else
			{
				add_moves(get_own_threats(ThreatType::OPEN_3));
				add_moves(get_own_threats(ThreatType::HALF_OPEN_3));
				add_moves(get_opponent_threats(ThreatType::HALF_OPEN_4));
				add_moves(get_opponent_threats(ThreatType::OPEN_3));
				add_moves(get_opponent_threats(ThreatType::HALF_OPEN_3));
				generator_stage = REMAINING_MOVES;
			}
			if (actions.size() > initial_action_count)
				return; // we have generated some new moves
		}

		if (generator_stage == REMAINING_MOVES)
		{
			generator_stage = ALL_GENERATED;
			mark_neighborhood();
			create_remaining_moves();
		}

		assert(generator_stage == ALL_GENERATED);
	}
	void MoveGenerator::generateAll()
	{
		actions.is_in_danger = false;
		actions.is_threatening = false;
		actions.clear();
		switch (mode)
		{
			case MoveGeneratorMode::LEGAL:
			{
				moves = pattern_calculator.getLegalMovesMask();
				break;
			}
			case MoveGeneratorMode::REDUCED:
			case MoveGeneratorMode::NORMAL:
			case MoveGeneratorMode::THREATS:
			{
				moves.fill(false);
				if (mode == MoveGeneratorMode::NORMAL or mode == MoveGeneratorMode::THREATS)
				{
					if (try_win_in_1())
						break;
					if (defend_loss_in_2())
						break;
					if (try_win_in_3())
						break;
					if (defend_loss_in_4())
						break;
					if (try_win_in_5())
						break;
					if (defend_loss_in_6())
						break;
					add_moves(get_own_threats(ThreatType::HALF_OPEN_4));
					add_moves(get_own_threats(ThreatType::OPEN_3));
				}
				if (mode == MoveGeneratorMode::REDUCED)
				{
					add_moves(get_own_threats(ThreatType::FIVE));
					add_moves(get_opponent_threats(ThreatType::FIVE));
					add_moves(get_own_threats(ThreatType::OPEN_4));
					add_moves(get_own_threats(ThreatType::FORK_4x4));
					add_moves(get_opponent_threats(ThreatType::OPEN_4));
					add_moves(get_opponent_threats(ThreatType::FORK_4x4));
					add_moves(get_own_threats(ThreatType::FORK_4x3));
					add_moves(get_own_threats(ThreatType::FORK_3x3));
					add_moves(get_own_threats(ThreatType::HALF_OPEN_4));
					add_moves(get_own_threats(ThreatType::OPEN_3));
				}

				if (mode == MoveGeneratorMode::REDUCED or mode == MoveGeneratorMode::NORMAL)
				{
					mark_neighborhood();
					create_remaining_moves();
					break;
				}
			}
		}
		generator_stage = ALL_GENERATED;

//		if (game_config.rules == GameRules::RENJU and get_own_sign() == Sign::CROSS)
//		{ // exclude moves that are forbidden in renju rule
//			unmark_move(get_own_threats(ThreatType::FORK_4x4));
//
//			const std::vector<Move> &own_fork_3x3 = get_own_threats(ThreatType::FORK_3x3);
//			for (auto iter = own_fork_3x3.begin(); iter < own_fork_3x3.end(); iter++)
//				if (at(iter->row, iter->col) and pattern_calculator.isForbidden(Sign::CROSS, iter->row, iter->col))
//					unmark_move(*iter);
//		}

		if (actions.size() == 0)
		{
			std::cout << "sign to move " << pattern_calculator.getSignToMove() << '\n';
			std::cout << pattern_calculator.getCurrentDepth() << '\n';
			pattern_calculator.print();
			pattern_calculator.printAllThreats();
			for (int row = 0; row < game_config.rows; row++)
			{
				for (int col = 0; col < game_config.cols; col++)
					switch (pattern_calculator.signAt(row, col))
					{
						case Sign::NONE:
							std::cout << (moves.at(row, col) ? " !" : " _");
							break;
						case Sign::CROSS:
							std::cout << " X";
							break;
						case Sign::CIRCLE:
							std::cout << " O";
							break;
						default:
							break;
					}
				std::cout << '\n';
			}
			std::cout << "no moves generated\n";
			exit(-1);
		}

//		std::cout << '\n';
//		for (int row = 0; row < game_config.rows; row++)
//		{
//			for (int col = 0; col < game_config.cols; col++)
//				if (at(row, col))
//					std::cout << " !";
//				else
//					std::cout << " _";
//			std::cout << '\n';
//		}
//		exit(0);
//		for (int row = 0; row < game_config.rows; row++)
//			std::cout << std::bitset<32>(moves[row]).to_string() << '\n';
		if (mode != MoveGeneratorMode::THREATS)
			assert(actions.size() > 0);
	}
	/*
	 * private
	 */
	void MoveGenerator::add_move(Move m) noexcept
	{
		add_move(Location(m.row, m.col));
	}
	void MoveGenerator::add_move(Location m) noexcept
	{
		if (moves.at(m.row, m.col) == false)
		{ // a move should not be added twice
			actions.add(Move(get_own_sign(), m.row, m.col));
			moves.at(m.row, m.col) = true;
		}
	}
	void MoveGenerator::add_moves(const std::vector<Location> &moves) noexcept
	{
		for (auto iter = moves.begin(); iter < moves.end(); iter++)
			add_move(*iter);
	}
	int MoveGenerator::create_defensive_moves(Location move, Direction dir)
	{
		const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), move.row, move.col, dir);
		if (is_anything_forbidden_for(get_own_sign()))
		{
			int count = 0;
			for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
				if (not pattern_calculator.isForbidden(get_own_sign(), iter->row, iter->col))
				{
					add_move(*iter);
					count++;
				}
			return count;
		}
		else
		{
			for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)
				add_move(*iter);
			return defensive_moves.size();
		}
	}
	void MoveGenerator::create_remaining_moves()
	{
		const Sign sign = get_own_sign();
		for (int row = 0; row < game_config.rows; row++)
		{
			uint32_t tmp = moves[row];
//			if (is_anything_forbidden_for(get_own_sign()))
//			{
//				for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
//					if ((tmp & 1) and not pattern_calculator.isForbidden(sign, row, col))
//						actions.add(Move(row, col, sign));
//			}
//			else
//			{
				for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
					if (tmp & 1)
						actions.add(Move(row, col, sign));
//			}
		}
	}
	void MoveGenerator::mark_neighborhood()
	{
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
		BitMask2D<uint32_t, 32> neighboring_moves(row_offset + game_config.rows + row_offset + 1, game_config.cols);

		for (int row = 0; row < game_config.rows; row++)
		{
			uint32_t tmp = legal_moves[row];
			for (int col = 0; col < game_config.cols; col++, tmp >>= 1)
				if ((tmp & 1) == 0)
				{ // the spot is not legal -> it is occupied
#ifdef __AVX2__
					const __m256i shift = _mm256_set1_epi32(28 - col);
					const __m256i x = _mm256_loadu_si256((__m256i*) (neighboring_moves.data() + row));
					_mm256_storeu_si256((__m256i*) (neighboring_moves.data() + row), _mm256_or_si256(x, _mm256_srlv_epi32(masks, shift)));
#elif defined(__SSE2__)
					const __m128i shift = _mm_set_epi32(0, 0, 0, 28 - col);
					const __m128i x1 = _mm_loadu_si128((__m128i*) (neighboring_moves.data() + row));
					const __m128i x2 = _mm_loadu_si128((__m128i*) (neighboring_moves.data() + row + 4));
					_mm_storeu_si128((__m128i*) (neighboring_moves.data() + row), _mm_or_si128(x1, _mm_srl_epi32(masks1, shift)));
					_mm_storeu_si128((__m128i*) (neighboring_moves.data() + row + 4), _mm_or_si128(x2, _mm_srl_epi32(masks2, shift)));
#else
					const int shift = 28 - col; // now calculate how much the masks need to be shifted to the right to match desired position
					for (int i = 0; i < 7; i++)
						neighboring_moves[row + i] |= (neighboring_moves[i] >> shift);// now just apply patterns to the board
#endif
				}
		}
		if (pattern_calculator.getCurrentDepth() == 0) // no moves were made on board
			neighboring_moves.at(row_offset + game_config.rows / 2, game_config.cols / 2) = true; // mark single move at the center

		// moves[i] is a mask of moves that has been added until this point
		// so (~moves[i]) is a mask of moves that has not been added
		// neighboring_moves[row_offset + i] is a mask of all neighboring moves to all stones already on board
		// but they may be illegal, so it is masked with legal_moves[i]
		// and it is assigned to the moves[i] because this array will be used for adding all moves that are left, after then it is no longer necessary
		for (int i = 0; i < game_config.rows; i++)
			moves[i] = (~moves[i]) & (neighboring_moves[row_offset + i] & legal_moves[i]);
	}
	bool MoveGenerator::try_win_in_1()
	{
		const std::vector<Location> &own_fives = get_own_threats(ThreatType::FIVE);
		if (own_fives.size() > 0)
		{
			actions.is_in_danger = false;
			actions.is_threatening = true;
			add_moves(own_fives);
			return true;
		}
		return false;
	}
	bool MoveGenerator::defend_loss_in_2()
	{
		const std::vector<Location> &opponent_fives = get_opponent_threats(ThreatType::FIVE);
		if (opponent_fives.size() > 0)
		{
			actions.is_in_danger = true;
			actions.is_threatening = false;
			int move_count = 0;
			for (auto move = opponent_fives.begin(); move < opponent_fives.end(); move++)
			{
				const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
				const Direction direction = patterns.findDirectionOf(PatternType::FIVE);
				move_count += create_defensive_moves(*move, direction);
			}
			if (move_count == 0)
				add_moves(opponent_fives); // we must provide some moves to play even if the threat is not refutable
			return true;
		}
		return false;
	}
	bool MoveGenerator::try_win_in_3()
	{
		const std::vector<Location> &own_open_four = get_own_threats(ThreatType::OPEN_4);
		const std::vector<Location> &own_fork_4x4 = get_own_threats(ThreatType::FORK_4x4);
		const int own_win_in_3_count = own_open_four.size() + (is_own_4x4_fork_forbidden() ? 0 : own_fork_4x4.size());
		if (own_win_in_3_count > 0)
		{
			actions.is_in_danger = false;
			actions.is_threatening = true;
			add_moves(own_open_four);
			if (not is_own_4x4_fork_forbidden())
				add_moves(own_fork_4x4);
			return true;
		}
		return false;
	}
	bool MoveGenerator::defend_loss_in_4()
	{
		const std::vector<Location> &opp_open_four = get_opponent_threats(ThreatType::OPEN_4);
		for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
		{
			const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
			const Direction direction = patterns.findDirectionOf(PatternType::OPEN_4);
			create_defensive_moves(*move, direction);
		}

		const std::vector<Location> &opp_fork_4x4 = get_opponent_threats(ThreatType::FORK_4x4);
		if (not is_opponent_4x4_fork_forbidden())
		{
			for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
			{
				const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
				for (Direction direction = 0; direction < 4; direction++)
					if (patterns[direction] == PatternType::OPEN_4 or patterns[direction] == PatternType::HALF_OPEN_4
							or patterns[direction] == PatternType::DOUBLE_4)
						create_defensive_moves(*move, direction);
			}
		}

		const int opp_win_in_3_count = opp_open_four.size() + (is_opponent_4x4_fork_forbidden() ? 0 : opp_fork_4x4.size());
		if (opp_win_in_3_count > 0)
		{
			actions.is_in_danger = true;
			actions.is_threatening = false;
			add_moves(get_own_threats(ThreatType::HALF_OPEN_4)); // half open four is a threat of win in 1
			add_moves(get_own_threats(ThreatType::FORK_4x3)); // 4x3 fork also contains a half open four
			return true;
		}
		return false;
	}
	bool MoveGenerator::try_win_in_5()
	{
		const std::vector<Location> &own_fork_4x3 = get_own_threats(ThreatType::FORK_4x3);
		bool flag = false;
		if (own_fork_4x3.size() > 0)
		{
			actions.is_in_danger = false;
			actions.is_threatening = true;
			add_moves(get_own_threats(ThreatType::FORK_4x3));
			for (auto iter = own_fork_4x3.begin(); iter < own_fork_4x3.end(); iter++)
				if (is_4x3_fork_winning(iter->row, iter->col, get_own_sign()))
					flag = true;
		}

		if (pattern_calculator.getThreatHistogram(get_opponent_sign()).hasAnyFour() == false)
		{
			const std::vector<Location> &own_fork_3x3 = get_copy(get_own_threats(ThreatType::FORK_3x3));
			for (auto move = own_fork_3x3.begin(); move < own_fork_3x3.end(); move++)
				if (not is_own_3x3_fork_forbidden(*move))
				{
					flag = true;
					actions.is_in_danger = false;
					actions.is_threatening = true;
					add_move(*move);
				}
		}
		return flag;
	}
	bool MoveGenerator::defend_loss_in_6()
	{
//		int opp_4x3_fork_count = 0;
//		const std::vector<Location> &opp_fork_4x3 = get_opponent_threats(ThreatType::FORK_4x3);
//		for (auto move = opp_fork_4x3.begin(); move < opp_fork_4x3.end(); move++)
//			if (is_4x3_fork_winning(move->row, move->col, get_opponent_sign()))
//			{ // handle direct defensive moves
//				opp_4x3_fork_count++;
//				const DirectionGroup<PatternType> patterns = pattern_calculator.getPatternTypeAt(get_opponent_sign(), move->row, move->col);
//				for (Direction direction = 0; direction < 4; direction++)
//				{
//					if (patterns[direction] == PatternType::HALF_OPEN_4)
//					{
//						create_defensive_moves(*move, direction); // mark direct defensive move
//						// find indirect defensive moves (those creating open and half-open three)
//						const ShortVector<Location, 6> defensive_moves = pattern_calculator.getDefensiveMoves(get_own_sign(), move->row, move->col,
//								direction);
//						for (auto iter = defensive_moves.begin(); iter < defensive_moves.end(); iter++)	// check the surroundings of the half open four from 4x3 fork
//						{
//							for (Direction dir = 0; dir < 4; dir++) // check all directions around the response to the half open four
//								for (int j = -4; j <= 4; j++) // check the surroundings of the response
//								{
//									const int x = iter->row + j * get_row_step(dir);
//									const int y = iter->col + j * get_col_step(dir);
//									if (0 <= x and x < game_config.rows and 0 <= y and y < game_config.cols)
//									{ // if inside board
//										const PatternType pt = pattern_calculator.getPatternTypeAt(get_own_sign(), x, y, dir);
//										if (pt == PatternType::HALF_OPEN_3 or pt == PatternType::OPEN_3)
//											add_move(Location(iter->row + j * get_row_step(dir), iter->col + j * get_col_step(dir)));
//									}
//								}
//						}
//					}
//					if (patterns[direction] == PatternType::OPEN_3)
//						create_defensive_moves(*move, direction);
//				}
//			}
//
//		if (opp_4x3_fork_count > 0)
//		{
//			actions.is_in_danger = true;
//			actions.is_threatening = false;
//			add_moves(get_own_threats(ThreatType::HALF_OPEN_4)); // half open four is a threat of win in 1 (may help gain initiative)
//			add_moves(get_own_threats(ThreatType::FORK_4x3)); // half open four is a threat of win in 1 (may help gain initiative)
//			return true;
//		}
		return false;
	}
	bool MoveGenerator::is_4x3_fork_winning(int row, int col, Sign sign)
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
	bool MoveGenerator::is_move_valid(Move m) const noexcept
	{
		return m.sign == get_own_sign() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

} /* namespace ag */
