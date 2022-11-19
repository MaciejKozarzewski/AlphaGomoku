/*
 * MoveGenerator.hpp
 *
 *  Created on: Oct 2, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_MOVEGENERATOR_HPP_
#define ALPHAGOMOKU_SOLVER_MOVEGENERATOR_HPP_

#include <alphagomoku/ab_search/ActionList.hpp>
#include <alphagomoku/ab_search/CommonBase.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/BitMask.hpp>

#include <cinttypes>
#include <bitset>

namespace ag
{
	class PatternCalculator;
}

namespace ag
{

	enum class MoveGeneratorMode
	{
		LEGAL,
		REDUCED, /* set of all legal moves reduced to a reasonable size by excluding moves that cannot change the situation (too far from already pladed stones) */
		NORMAL,
		THREATS /* equivalent to DEFENSIVE and ATTACKS combined */
	};

	class MoveGenerator: CommonBase
	{
			enum GenerationStage
			{
				NOT_STARTED,
				HASH_MOVE,
				FORCING_THREATS,
				KILLER_MOVES,
				WEAKER_THREATS,
				REMAINING_MOVES,
				ALL_GENERATED
			};

			BitMask2D<uint32_t, 32> moves;
			ActionList &actions;
			Move hash_move;
			ShortVector<Move, 4> killer_moves;
			GenerationStage generator_stage = NOT_STARTED;
			MoveGeneratorMode mode = MoveGeneratorMode::NORMAL;
		public:
			MoveGenerator(GameConfig gameConfig, PatternCalculator &calc, ActionList &actionList, MoveGeneratorMode mode);
			void setExternalStorage(std::vector<Location> &tmp) noexcept;
			void set(Move hashMove, const ShortVector<Move, 4> &killerMoves) noexcept;
			void generate();
			void generateAll();
		private:
			void add_move(Move move) noexcept;
			void add_move(Location location) noexcept;
			void add_moves(const std::vector<Location> &locations) noexcept;
			/*
			 * \brief Marks defensive moves and returns the number of them.
			 */
			int create_defensive_moves(Location location, Direction dir);
			void create_remaining_moves();
			void mark_neighborhood();
			bool try_win_in_1();
			bool defend_loss_in_2();
			bool try_win_in_3();
			bool defend_loss_in_4();
			bool try_win_in_5();
			bool defend_loss_in_6();
			bool is_4x3_fork_winning(int row, int col, Sign sign);
			bool is_move_valid(Move m) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_MOVEGENERATOR_HPP_ */
