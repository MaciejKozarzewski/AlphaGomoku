/*
 * MoveGenerator.hpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_MOVEGENERATOR_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_MOVEGENERATOR_HPP_

#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/BitMask.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <cinttypes>

namespace ag
{
	enum class ThreatType : int8_t;
	class PatternCalculator;
	class ActionList;
}

namespace ag
{
	enum class MoveGeneratorMode
	{
		BASIC, // only check game end conditions (win or draw)
		THREATS, // generate VCF threats
		OPTIMAL, // generate optimal list of non-losing moves
		REDUCED, // generate only local neighborhood of existing stones
		LEGAL // generate all legal moves
	};

	class MoveGenerator
	{
		private:
			typedef int AddMode;
			static constexpr AddMode EXCLUDE_DUPLICATE = 0;
			static constexpr AddMode OVERRIDE_DUPLICATE = 1;

			struct Result
			{
					bool must_continue = true;
					Score score;

					static Result canStopNow(Score s = Score()) noexcept
					{
						return Result( { false, s });
					}
					static Result mustContinue() noexcept
					{
						return Result( { true, Score() });
					}
			};
			GameConfig game_config;
			PatternCalculator &pattern_calculator;
			LocationList temporary_list;
			mutable std::vector<std::pair<Location, bool>> forbidden_moves_cache;

			BitMask2D<uint32_t, 32> moves;
			ActionList *actions = nullptr;

			TimedStat m_total_time;
			TimedStat m_defensive_moves;
			TimedStat m_draw_in_1;
			TimedStat m_win_in_1;
			TimedStat m_loss_in_2;
			TimedStat m_win_in_3;
			TimedStat m_loss_in_4;
			TimedStat m_win_in_5;
			TimedStat m_loss_in_6;
			TimedStat m_win_in_7;
			TimedStat m_forbidden_moves;
			TimedStat m_mark_neighborhood;
			TimedStat m_remaining_moves;
		public:
			MoveGenerator(GameConfig gameConfig, PatternCalculator &calc);
			~MoveGenerator();
			void setDrawAfter(int moves) noexcept;
			void setHashMove(Move m) noexcept;
			Score generate(ActionList &actions, MoveGeneratorMode mode);
		private:
			template<AddMode Mode>
			void add_move(Location loc, Score s = Score()) noexcept;
			template<AddMode Mode>
			void add_moves(const LocationList &locations, Score s = Score()) noexcept;
			template<AddMode Mode, typename T>
			void add_moves(const T begin, const T end, Score s = Score()) noexcept;
			StackVector<Location, 6> get_defensive_moves(Location location, Direction dir);
			Result try_draw_in_1();
			Result try_win_in_1();
			Result defend_loss_in_2();
			Result try_win_in_3();
			Result defend_loss_in_4();
			Result try_win_in_5();
			Result defend_loss_in_6();
			Result try_win_in_7();
			Score add_own_4x3_forks();
			void add_own_half_open_fours();
			Score try_solve_own_fork_4x3(Location move);
			void mark_forbidden_moves();
			BitMask2D<uint32_t, 32> mark_neighborhood();
			BitMask2D<uint32_t, 32> mark_star_like_pattern_for(Sign s);
			void create_remaining_moves(const BitMask2D<uint32_t, 32> &mask, Score s = Score());

			Sign get_own_sign() const noexcept;
			Sign get_opponent_sign() const noexcept;
			ThreatType get_own_threat_at(int row, int col) const noexcept;
			ThreatType get_opponent_threat_at(int row, int col) const noexcept;
			const LocationList& get_own_threats(ThreatType tt) const noexcept;
			const LocationList& get_opponent_threats(ThreatType tt) const noexcept;
			bool is_anything_forbidden_for(Sign sign) const noexcept;
			bool is_forbidden(Sign sign, Location loc) const noexcept;
			const LocationList& get_copy_of(const LocationList &list);
			bool is_move_valid(Move m) const noexcept;
			bool is_possible(Score s) const noexcept;
			int number_of_available_fours_for(Sign sign) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_MOVEGENERATOR_HPP_ */
