/*
 * PatternCalculator.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_PATTERNCALCULATOR_HPP_
#define ALPHAGOMOKU_SOLVER_PATTERNCALCULATOR_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/solver/PatternTable.hpp>
#include <alphagomoku/solver/ThreatTable.hpp>

#include <algorithm>
#include <cassert>

namespace ag::experimental
{
	typedef int Direction;
	const Direction HORIZONTAL = 0;
	const Direction VERTICAL = 1;
	const Direction DIAGONAL = 2;
	const Direction ANTIDIAGONAL = 3;

	struct RawPatternGroup
	{
			uint32_t horizontal = 0u;
			uint32_t vertical = 0u;
			uint32_t diagonal = 0u;
			uint32_t antidiagonal = 0u;

			uint32_t get(Direction dir) const noexcept
			{
				switch (dir)
				{
					case HORIZONTAL:
						return horizontal;
					case VERTICAL:
						return vertical;
					case DIAGONAL:
						return diagonal;
					case ANTIDIAGONAL:
						return antidiagonal;
					default:
						return 0u;
				}
			}
	};

	class ThreatHistogram
	{
			std::array<std::vector<Move>, 10> threats;
		public:
			ThreatHistogram()
			{
				for (size_t i = 1; i < threats.size(); i++) // do not reserve for 'NONE' threat
					threats[i].reserve(64);
			}
			const std::vector<Move>& get(ThreatType threat) const noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				return threats[static_cast<size_t>(threat)];
			}
			void remove(ThreatType threat, Move move) noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType::NONE)
				{
					std::vector<Move> &list = threats[static_cast<size_t>(threat)];
					const auto index = std::find(list.begin(), list.end(), move);
					assert(index != list.end()); // the threat must exist in the list
					list.erase(index);
				}
			}
			void add(ThreatType threat, Move move)
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType::NONE)
					threats[static_cast<size_t>(threat)].push_back(move);
			}
			void clear() noexcept
			{
				for (size_t i = 0; i < threats.size(); i++)
					threats[i].clear();
			}
			bool hasAnyFour() const noexcept
			{
				return get(ThreatType::HALF_OPEN_4).size() > 0 or get(ThreatType::FORK_4x3).size() > 0 or get(ThreatType::FORK_4x4).size() > 0
						or get(ThreatType::OPEN_4).size() > 0;
			}
			void print() const;
	};

	struct DefensiveMoves
	{
			std::array<BitMask<uint16_t>, 4> for_cross;
			std::array<BitMask<uint16_t>, 4> for_circle;
	};

	class PatternCalculator
	{
		private:
			GameConfig game_config;
			int pad;

			matrix<int16_t> internal_board;
			matrix<RawPatternGroup> raw_features;
			matrix<PatternTypeGroup> feature_types;
			matrix<DefensiveMoves> defensive_moves;

			matrix<Threat> threat_types;

			std::array<PatternEncoding, 4> central_spot_encoding;

			ThreatHistogram cross_threats;
			ThreatHistogram circle_threats;

			const PatternTable *pattern_table = nullptr;
			const ThreatTable *threat_table = nullptr;

			TimedStat features_init;
			TimedStat features_class;
			TimedStat threats_init;
			TimedStat features_update;
			TimedStat threats_update;

		public:
			PatternCalculator(GameConfig gameConfig);
			void setBoard(const matrix<Sign> &board, bool = false);
			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;

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
				switch (sign)
				{
					default:
						return BitMask<uint16_t>();
					case Sign::CROSS:
						return defensive_moves.at(row, col).for_cross[dir];
					case Sign::CIRCLE:
						return defensive_moves.at(row, col).for_circle[dir];
				}
			}

			void printRawFeature(int row, int col) const;
			void printThreat(int row, int col) const;
			void printAllThreats() const;
			void print(Move lastMove = Move()) const;
			void print_stats() const;
		private:
			void calculate_raw_features() noexcept;
			void classify_feature_types() noexcept;
			void prepare_threat_lists();

			void update_central_spot(int row, int col, int mode) noexcept;
			void update_neighborhood(int row, int col) noexcept;
			void update_feature_types_and_threats(int row, int col, int direction) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_PATTERNCALCULATOR_HPP_ */
