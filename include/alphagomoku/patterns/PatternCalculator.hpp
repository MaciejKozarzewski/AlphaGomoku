/*
 * PatternCalculator.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/BitMask.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/patterns/ThreatHistogram.hpp>
#include <alphagomoku/patterns/common.hpp>

#include <algorithm>
#include <cassert>

namespace ag
{
	typedef int UpdateMode;
	const UpdateMode ADD_MOVE = 0;
	const UpdateMode UNDO_MOVE = 1;

	class PatternCalculator
	{
		private:
			GameConfig game_config;
			Sign sign_to_move = Sign::NONE;
			int current_depth = 0;

			BitMask2D<uint32_t, 32> legal_moves_mask;
			matrix<int16_t> internal_board;
			matrix<DirectionGroup<uint32_t>> raw_patterns;
			matrix<TwoPlayerGroup<PatternType>> pattern_types;

			matrix<ThreatEncoding> threat_types;

			DirectionGroup<PatternEncoding> central_spot_encoding;

			ThreatHistogram cross_threats;
			ThreatHistogram circle_threats;

			const PatternTable *pattern_table = nullptr;
			const ThreatTable *threat_table = nullptr;
			const DefensiveMoveTable *defensive_move_table = nullptr;

			TimedStat features_init;
			TimedStat features_class;
			TimedStat threats_init;
			TimedStat features_update;
			TimedStat threats_update;

		public:
			static constexpr int padding = 5;
			static constexpr int extended_padding = padding + 1;

			PatternCalculator(GameConfig gameConfig);
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;

			GameConfig getConfig() const noexcept
			{
				return game_config;
			}
			int getCurrentDepth() const noexcept
			{
				return current_depth;
			}
			const BitMask2D<uint32_t>& getLegalMovesMask() const noexcept
			{
				return legal_moves_mask;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			Sign signAt(int row, int col) const noexcept
			{
				return static_cast<Sign>(internal_board.at(extended_padding + row, extended_padding + col));
			}
			const ThreatHistogram& getThreatHistogram(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return cross_threats;
				else
					return circle_threats;
			}
			uint32_t getRawPatternAt(int row, int col, Direction dir) const noexcept
			{
				return (raw_patterns.at(extended_padding + row, extended_padding + col)[dir] >> 2) & 4194303u;
			}
			uint32_t getExtendedPatternAt(int row, int col, Direction dir) const noexcept
			{
				return raw_patterns.at(extended_padding + row, extended_padding + col)[dir];
			}
			DirectionGroup<PatternType> getPatternTypeAt(Sign sign, int row, int col) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return pattern_types.at(row, col).for_cross;
				else
					return pattern_types.at(row, col).for_circle;
			}
			PatternType getPatternTypeAt(Sign sign, int row, int col, Direction dir) const noexcept
			{
				return getPatternTypeAt(sign, row, col)[dir];
			}
			ThreatType getThreatAt(Sign sign, int row, int col) const noexcept
			{
				switch (sign)
				{
					default:
						return ThreatType::NONE;
					case Sign::CROSS:
						return threat_types.at(row, col).forCross();
					case Sign::CIRCLE:
						return threat_types.at(row, col).forCircle();
				}
			}
			ShortVector<Move, 5> getDefensiveMoves(Sign sign, int row, int col, Direction dir) const noexcept
			{
				const uint32_t extended_pattern = getExtendedPatternAt(row, col, dir);
				const PatternType threat_to_defend = getPatternTypeAt(invertSign(sign), row, col, dir);
				const BitMask1D<uint16_t> tmp = defensive_move_table->getMoves(extended_pattern, sign, threat_to_defend);
				ShortVector<Move, 5> result;
				for (int i = -extended_padding; i <= extended_padding; i++)
					if (tmp[i + extended_padding])
						result.add(Move(row + i * get_row_step(dir), col + i * get_col_step(dir)));
				return result;
			}
			bool isForbidden(Sign sign, int row, int col) noexcept;

			void printRawFeature(int row, int col) const;
			void printThreat(int row, int col) const;
			void printAllThreats() const;
			void print(Move lastMove = Move()) const;
			void print_stats() const;
		private:
			void calculate_raw_features() noexcept;
			void classify_feature_types() noexcept;
			void prepare_threat_lists();

			void update_central_spot(int row, int col, UpdateMode mode) noexcept;
			void update_neighborhood(int row, int col) noexcept;
			void update_feature_types_and_threats(int row, int col, Direction direction) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_HPP_ */
