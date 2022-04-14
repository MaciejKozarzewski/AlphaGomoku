/*
 * FeatureExtractor_v3.hpp
 *
 *  Created on: Apr 11, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V3_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V3_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/vcf_solver/FeatureTable_v3.hpp>
#include <alphagomoku/vcf_solver/ThreatTable.hpp>

#include <alphagomoku/vcf_solver/FeatureExtractor_v2.hpp>

#include <cassert>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	class ThreatHistogram
	{
			std::array<std::vector<Move>, 9> threats;
		public:
			ThreatHistogram()
			{
				for (size_t i = 1; i < threats.size(); i++) // do not reserve for 'NONE' threat
					threats[i].reserve(64);
			}
			const std::vector<Move>& get(ThreatType_v3 threat) const noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				return threats[static_cast<size_t>(threat)];
			}
			void remove(ThreatType_v3 threat, Move move) noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType_v3::NONE)
				{
					std::vector<Move> &list = threats[static_cast<size_t>(threat)];
					const auto index = std::find(list.begin(), list.end(), move);
					assert(index != list.end()); // the threat must exist in the list
					list.erase(index);
				}
			}
			void add(ThreatType_v3 threat, Move move) noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType_v3::NONE)
					threats[static_cast<size_t>(threat)].push_back(move);
			}
			void clear() noexcept
			{
				for (size_t i = 0; i < threats.size(); i++)
					threats[i].clear();
			}
			void print() const;
	};

	class FeatureExtractor_v3
	{
		private:
			GameConfig game_config;
			int pad;

			Sign sign_to_move = Sign::NONE;
			int root_depth = 0;
			matrix<int16_t> internal_board;
			matrix<FeatureGroup> raw_features;
			matrix<FeatureTypeGroup> feature_types;
			matrix<Threat_v3> threat_types;
			FeatureEncoding central_spot_encoding[4];

			ThreatHistogram cross_threats;
			ThreatHistogram circle_threats;

			TimedStat features_init;
			TimedStat features_class;
			TimedStat threats_init;
			TimedStat features_update;
			TimedStat threats_update;

			std::vector<uint64_t> feature_value_statistics;
			std::vector<uint8_t> feature_value_table;

			const FeatureTable_v3 *feature_table = nullptr;
			const ThreatTable *threat_table = nullptr;

		public:
			FeatureExtractor_v3(GameConfig gameConfig);

			Sign signAt(int row, int col) const noexcept
			{
				return static_cast<Sign>(internal_board.at(pad + row, pad + col));
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			int getRootDepth() const noexcept
			{
				return root_depth;
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove);

			const ThreatHistogram& getThreatHistogram(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				if (sign == Sign::CROSS)
					return cross_threats;
				else
					return circle_threats;
			}

			int8_t getValueAt(Sign sign, int row, int col) const noexcept;
			uint32_t getRawFeatureAt(int row, int col, Direction dir) const noexcept;
			FeatureType getFeatureTypeAt(Sign sign, int row, int col, Direction dir) const noexcept;
			ThreatType_v3 getThreatAt(Sign sign, int row, int col) const noexcept;

			void printRawFeature(int row, int col) const;
			void printThreat(int row, int col) const;
			void printAllThreats() const;
			void print() const;

			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;
			void updateValueStatistics(Move move) noexcept;
			uint8_t getValue(Move move) const noexcept;

			void print_stats() const;
		private:
			void calculate_raw_features() noexcept;
			void classify_feature_types() noexcept;
			void get_threat_lists();

			void update_central_spot(int row, int col, int mode) noexcept;
			void update_feature_types_and_threats(int row, int col, int direction) noexcept;
	};

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V3_HPP_ */
