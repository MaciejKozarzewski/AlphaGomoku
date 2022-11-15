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
	namespace tss
	{
		class ActionList;
	}
}

namespace ag
{
	namespace tss
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
						bool can_continue = true;
						Score score;
						Result() noexcept = default;
						Result(bool c) noexcept :
								can_continue(c)
						{
						}
						Result(bool c, Score s) noexcept :
								can_continue(c),
								score(s)
						{
						}
				};
				GameConfig game_config;
				PatternCalculator &pattern_calculator;
				std::vector<Location> temporary_list;

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
				Result try_win_in_1();
				Result try_draw_in_1();
				Result defend_loss_in_2();
				Result try_win_in_3();
				Result defend_loss_in_4();
				Result try_win_in_5();
				Score add_own_4x3_forks();
				int add_own_half_open_fours();
				Score try_solve_own_fork_4x3(Location move);
				Score try_solve_foul_attack(Location move);

				bool is_caro_rule() const noexcept;
				Sign get_own_sign() const noexcept;
				Sign get_opponent_sign() const noexcept;
				ThreatType get_own_threat_at(int row, int col) const noexcept;
				ThreatType get_opponent_threat_at(int row, int col) const noexcept;
				const std::vector<Location>& get_own_threats(ThreatType tt) const noexcept;
				const std::vector<Location>& get_opponent_threats(ThreatType tt) const noexcept;
				bool is_own_4x4_fork_forbidden() const noexcept;
				bool is_opponent_4x4_fork_forbidden() const noexcept;
				bool is_own_3x3_fork_forbidden(Location m) const noexcept;
				bool is_opponent_3x3_fork_forbidden(Location m) const noexcept;
				bool is_anything_forbidden_for(Sign sign) const noexcept;
				const std::vector<Location>& get_copy(const std::vector<Location> &list);
				bool is_move_valid(Move m) const noexcept;
		};
	}
} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_THREATGENERATOR_HPP_ */
