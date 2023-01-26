/*
 * ThreatGenerator.hpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_THREATGENERATOR_HPP_
#define ALPHAGOMOKU_TSS_THREATGENERATOR_HPP_

#include <alphagomoku/tss/Score.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/BitMask.hpp>

#include <cinttypes>

namespace ag
{
	enum class ThreatType : int8_t;
	class PatternCalculator;
	class ActionList;
}

namespace ag
{
	enum class GeneratorMode
	{
		BASIC,
		STATIC,
		VCF,
		VCT
	};

	class ThreatGenerator
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
			std::vector<Location> temporary_list;
			mutable std::vector<std::pair<Location, bool>> forbidden_moves_cache;

			BitMask2D<uint32_t, 32> moves;
			ActionList *actions = nullptr;
			int number_of_moves_for_draw;
		public:
			ThreatGenerator(GameConfig gameConfig, PatternCalculator &calc);
			void setDrawAfter(int moves) noexcept;
			void setHashMove(Move m) noexcept;
			void setKillerMoves(const ShortVector<Move, 4> &killers) noexcept;
			Score generate(ActionList &actions, GeneratorMode mode);
		private:
			template<AddMode Mode>
			void add_move(Location location, Score s = Score()) noexcept;
			template<AddMode Mode>
			void add_moves(const std::vector<Location> &locations, Score s = Score()) noexcept;
			template<AddMode Mode, typename T>
			void add_moves(const T begin, const T end, Score s = Score()) noexcept;
			ShortVector<Location, 6> get_defensive_moves(Location location, Direction dir);
			Result try_draw_in_1();
			Result try_win_in_1();
			Result defend_loss_in_2();
			Result try_win_in_3();
			Result defend_loss_in_4();
			Result try_win_in_5();
			Score add_own_4x3_forks();
			int add_own_half_open_fours();
			Score try_solve_own_fork_4x3(Location move);

			Sign get_own_sign() const noexcept;
			Sign get_opponent_sign() const noexcept;
			ThreatType get_own_threat_at(int row, int col) const noexcept;
			ThreatType get_opponent_threat_at(int row, int col) const noexcept;
			const std::vector<Location>& get_own_threats(ThreatType tt) const noexcept;
			const std::vector<Location>& get_opponent_threats(ThreatType tt) const noexcept;
			bool is_anything_forbidden_for(Sign sign) const noexcept;
			bool is_forbidden(Sign sign, Location loc) const noexcept;
			const std::vector<Location>& get_copy_of(const std::vector<Location> &list);
			bool is_move_valid(Move m) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_THREATGENERATOR_HPP_ */
