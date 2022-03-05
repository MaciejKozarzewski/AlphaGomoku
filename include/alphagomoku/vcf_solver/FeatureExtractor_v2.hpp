/*
 * FeatureExtractor_v2.hpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V2_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V2_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

#include <unordered_map>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	struct FeatureGroup
	{
			uint32_t horizontal;
			uint32_t vertical;
			uint32_t diagonal;
			uint32_t antidiagonal;

			uint32_t get(Direction dir) const noexcept
			{
				switch (dir)
				{
					case Direction::HORIZONTAL:
						return horizontal;
					case Direction::VERTICAL:
						return vertical;
					case Direction::DIAGONAL:
						return diagonal;
					case Direction::ANTIDIAGONAL:
						return antidiagonal;
					default:
						return 0;
				}
			}
//			FeatureDescriptor horizontal;
//			FeatureDescriptor vertical;
//			FeatureDescriptor diagonal;
//			FeatureDescriptor antidiagonal;
	};

	struct ThreatGroup
	{
			ThreatType horizontal;
			ThreatType vertical;
			ThreatType diagonal;
			ThreatType antidiagonal;

			ThreatType best() const noexcept
			{
				return std::max(std::max(horizontal, vertical), std::max(diagonal, antidiagonal));
			}
			ThreatType get(Direction dir) const noexcept
			{
				switch (dir)
				{
					case Direction::HORIZONTAL:
						return horizontal;
					case Direction::VERTICAL:
						return vertical;
					case Direction::DIAGONAL:
						return diagonal;
					case Direction::ANTIDIAGONAL:
						return antidiagonal;
					default:
						return ThreatType::NONE;
				}
			}
			ThreatType& get(Direction dir) noexcept
			{
				switch (dir)
				{
					default:
					case Direction::HORIZONTAL:
						return horizontal;
					case Direction::VERTICAL:
						return vertical;
					case Direction::DIAGONAL:
						return diagonal;
					case Direction::ANTIDIAGONAL:
						return antidiagonal;
				}
			}
	};

	class FeatureExtractor_v2
	{
		private:
			GameConfig game_config;
			int pad;

			Sign sign_to_move = Sign::NONE;
			int root_depth = 0;
			matrix<int> internal_board;
			matrix<FeatureGroup> features;
			matrix<ThreatGroup> cross_threats;
			matrix<ThreatGroup> circle_threats;

			std::vector<Move> cross_five;
			std::vector<Move> cross_open_four;
			std::vector<Move> cross_half_open_four;

			std::vector<Move> circle_five;
			std::vector<Move> circle_open_four;
			std::vector<Move> circle_half_open_four;
		public:
			FeatureExtractor_v2(GameConfig gameConfig);

			Sign signAt(int row, int col) const noexcept;
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			Sign getSignToMove() const noexcept;
			int getRootDepth() const noexcept;

			const std::vector<Move>& getThreats(ThreatType type, Sign sign) const noexcept;

			uint32_t getFeatureAt(int row, int col, Direction dir) const noexcept;
			ThreatType getThreatAt(Sign sign, int row, int col, Direction dir) const noexcept;

			void printFeature(int row, int col) const;
			void printThreat(int row, int col) const;
			void printAllThreats() const;
			void print() const;

			void addMove(Move move) noexcept;
			void undoMove(Move move) noexcept;

			void print_stats() const;
		private:
			uint32_t get_feature_at(int row, int col, Direction dir) const noexcept;
			void calc_all_features() noexcept;
			void get_threat_lists();

			void update_threats(int row, int col);
			void update_threat_at(const FeatureTable &table, int row, int col, Direction direction);
			void update_threat_list(ThreatType old_threat, ThreatType new_threat, Move move, std::vector<Move> &five, std::vector<Move> &open_four,
					std::vector<Move> &half_open_four);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V2_HPP_ */
