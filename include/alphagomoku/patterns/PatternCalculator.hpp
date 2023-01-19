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
#include <alphagomoku/patterns/RawPatternCalculator.hpp>
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

	template<typename T>
	struct Change
	{
			T previous = T { };
			T current = T { };
			Location location;
			Change() noexcept = default;
			Change(T prev, T curr, Location loc) :
					previous(prev),
					current(curr),
					location(loc)
			{
			}
	};

	class PatternCalculator
	{
		public:
			static constexpr int padding = 5;
			static constexpr int extended_padding = padding + 1;
		private:
			GameConfig game_config;
			Sign sign_to_move = Sign::NONE;
			int current_depth = 0;

			BitMask2D<uint32_t, 32> legal_moves_mask;
			matrix<Sign> internal_board;
			RawPatternCalculator raw_patterns;
			matrix<TwoPlayerGroup<DirectionGroup<PatternType>>> pattern_types;

			matrix<ThreatEncoding> threat_types;

			ThreatHistogram cross_threats;
			ThreatHistogram circle_threats;

			const PatternTable *pattern_table = nullptr; // non-owning
			const ThreatTable *threat_table = nullptr; // non-owning
			const DefensiveMoveTable *defensive_move_table = nullptr; // non-owning

			std::vector<Change<ThreatEncoding>> changed_threats;
			Change<Sign> changed_moves;

			TimedStat features_init;
			TimedStat features_class;
			TimedStat threats_init;
			TimedStat features_update;
			TimedStat threats_update;
		public:

			PatternCalculator(GameConfig gameConfig);
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void encodeInputFeatures(matrix<uint32_t> &features);

			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;

			Change<Sign> getChangeOfMoves() const noexcept
			{
				return changed_moves;
			}
			const std::vector<Change<ThreatEncoding>>& getChangeOfThreats() const noexcept
			{
				return changed_threats;
			}
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
				return internal_board.at(row, col);
			}
			const ThreatHistogram& getThreatHistogram(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return cross_threats;
				else
					return circle_threats;
			}
			NormalPattern getNormalPatternAt(int row, int col, Direction dir) const noexcept
			{
				return raw_patterns.getNormalPatternAt(row, col, dir);
			}
			ExtendedPattern getExtendedPatternAt(int row, int col, Direction dir) const noexcept
			{
				return raw_patterns.getExtendedPatternAt(row, col, dir);
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
			ThreatEncoding getThreatAt(int row, int col) const noexcept
			{
				return threat_types.at(row, col);
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
			ShortVector<Location, 6> getDefensiveMoves(Sign defenderSign, int row, int col, Direction dir) const noexcept
			{
				const ExtendedPattern extended_pattern = getExtendedPatternAt(row, col, dir);
				const PatternType threat_to_defend = getPatternTypeAt(invertSign(defenderSign), row, col, dir);
				const BitMask1D<uint16_t> tmp = defensive_move_table->getMoves(extended_pattern, defenderSign, threat_to_defend);
				ShortVector<Location, 6> result;
				for (int i = -extended_padding; i <= extended_padding; i++)
					if (tmp[extended_padding + i]) // the defensive move will not be outside the board
						result.add(shiftInDirection(dir, i, Location(row, col)));
				return result;
			}
			bool isForbidden(Sign sign, int row, int col) noexcept;

			void printRawFeature(int row, int col) const;
			void printThreat(int row, int col) const;
			void printAllThreats() const;
			void print(Move lastMove = Move()) const;
			void print_stats() const;
		private:
			void classify_feature_types() noexcept;
			void prepare_threat_lists();
			void update_around(int row, int col, Sign s, UpdateMode mode) noexcept;
			void update_feature_types_and_threats(int row, int col, Direction direction, int mode) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_HPP_ */
