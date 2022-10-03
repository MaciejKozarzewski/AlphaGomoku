/*
 * PatternCalculator_v2.hpp
 *
 *  Created on: Oct 3, 2022
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_V2_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_V2_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/patterns/PatternTable_v2.hpp>
#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/solver/BitBoard.hpp>

#include <algorithm>
#include <cassert>

namespace ag::experimental
{
	class PatternCalculator_v2
	{
		private:
			GameConfig game_config;
			int pad;
			Sign sign_to_move = Sign::NONE;

			BitBoard<uint32_t> legal_moves_mask;
			matrix<int16_t> internal_board;
			matrix<RawPatternGroup> raw_features;
			matrix<PatternTypeGroup> feature_types;

			matrix<Threat> threat_types;

			std::array<BitMask<uint16_t>, 4> central_spot_liberties;

			ThreatHistogram cross_threats;
			ThreatHistogram circle_threats;

			const PatternTable_v2 *pattern_table = nullptr;
			const ThreatTable *threat_table = nullptr;

			TimedStat features_init;
			TimedStat features_class;
			TimedStat threats_init;
			TimedStat features_update;
			TimedStat threats_update;

			size_t neighborhood_updates = 0;

		public:
			PatternCalculator_v2(GameConfig gameConfig);
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;
			int getMoveValue(Move move, const std::array<int, 4> &values) const noexcept;

			const BitBoard<uint32_t>& getLegalMovesMask() const noexcept
			{
				return legal_moves_mask;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			int getPadding() const noexcept
			{
				return pad;
			}
			Sign signAt(int row, int col) const noexcept
			{
				return static_cast<Sign>(internal_board.at(pad + row, pad + col));
			}
			const ThreatHistogram& getThreatHistogram(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return cross_threats;
				else
					return circle_threats;
			}
			uint32_t getRawFeatureAt(int row, int col, Direction dir) const noexcept
			{
				return raw_features.at(pad + row, pad + col).get(dir);
			}
			std::array<PatternType, 4> getPatternTypeAt(Sign sign, int row, int col) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return feature_types.at(row, col).for_cross;
				else
					return feature_types.at(row, col).for_circle;
			}
			PatternType getPatternTypeAt(Sign sign, int row, int col, Direction dir) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return feature_types.at(row, col).for_cross[dir];
				else
					return feature_types.at(row, col).for_circle[dir];
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
			BitMask<uint16_t> getDefensiveMoves(Sign sign, int row, int col, Direction dir) const noexcept
			{
				return pattern_table->getDefensiveMoves(getRawFeatureAt(row, col, dir), sign);
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

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNCALCULATOR_V2_HPP_ */
