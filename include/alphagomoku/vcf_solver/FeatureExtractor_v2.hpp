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
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

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
	};

	struct ThreatGroup
	{
			Threat horizontal;
			Threat vertical;
			Threat diagonal;
			Threat antidiagonal;

			Threat best() const noexcept
			{
//				ThreatType cross = get_best_threat(horizontal.for_cross, vertical.for_cross, diagonal.for_cross, antidiagonal.for_cross);
//				ThreatType circle = get_best_threat(horizontal.for_circle, vertical.for_circle, diagonal.for_circle, antidiagonal.for_circle);
//				return Threat(cross, circle);
				return max(max(horizontal, vertical), max(diagonal, antidiagonal));
			}
			Threat get(Direction dir) const noexcept
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
						return Threat();
				}
			}
			Threat& get(Direction dir) noexcept
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
		private:
//			ThreatType get_best_threat(ThreatType dir1, ThreatType dir2, ThreatType dir3, ThreatType dir4) const noexcept
//			{
//				static const std::array<ThreatType, 256> table = get_best_threat_table();
//				return table[(int) dir1 + 4 * (int) dir2 + 16 * (int) dir3 + 64 * (int) dir4];
//			}
//			std::array<ThreatType, 256> get_best_threat_table() const noexcept
//			{
//				std::array<ThreatType, 256> result;
//				for (int dir1 = 0; dir1 < 4; dir1++)
//					for (int dir2 = 0; dir2 < 4; dir2++)
//						for (int dir3 = 0; dir3 < 4; dir3++)
//							for (int dir4 = 0; dir4 < 4; dir4++)
//							{
//								int max_val = std::max(std::max(dir1, dir2), std::max(dir3, dir4));
//								int fork = (dir1 == 1) + (dir2 == 1) + (dir3 == 1) + (dir4 == 1);
//								if (fork >= 2)
//									max_val = std::max(max_val, 2); // a fork counts as open four
//								int index = dir1 + 4 * dir2 + 16 * dir3 + 64 * dir4;
//								result[index] = static_cast<ThreatType>(max_val);
//							}
//				return result;
//			}
	};

	class FeatureExtractor_v2
	{
		private:
			GameConfig game_config;
			int pad;

			Sign sign_to_move = Sign::NONE;
			int root_depth = 0;
			matrix<int16_t> internal_board;
			matrix<FeatureGroup> features;
			matrix<ThreatGroup> threats;

			std::vector<Move> cross_five;
			std::vector<Move> cross_open_four;
			std::vector<Move> cross_half_open_four;

			std::vector<Move> circle_five;
			std::vector<Move> circle_open_four;
			std::vector<Move> circle_half_open_four;

		public:
			FeatureExtractor_v2(GameConfig gameConfig);

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

			const std::vector<Move>& getThreats(ThreatType type, Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				assert(type != ThreatType::NONE);
				switch (type)
				{
					default:
					case ThreatType::HALF_OPEN_FOUR:
						return (sign == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
					case ThreatType::OPEN_FOUR:
						return (sign == Sign::CROSS) ? cross_open_four : circle_open_four;
					case ThreatType::FIVE:
						return (sign == Sign::CROSS) ? cross_five : circle_five;
				}
			}

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
			void update_central_threat(const FeatureTable &table, int row, int col);

			void remove_single_threat(Move move, std::vector<Move> &list) noexcept
			{
				const auto index = std::find(list.begin(), list.end(), move);
				assert(index != list.end()); // the threat must exist in the list
				list.erase(index);
			}
			void add_single_threat(Move move, std::vector<Move> &list) noexcept
			{
				list.push_back(move);
			}

			template<Sign sign>
			void remove_threat(ThreatType threat, Move move) noexcept
			{
				switch (threat)
				{
					default:
						break;
					case ThreatType::HALF_OPEN_FOUR:
						remove_single_threat(move, (sign == Sign::CROSS) ? cross_half_open_four : circle_half_open_four);
						break;
					case ThreatType::OPEN_FOUR:
						remove_single_threat(move, (sign == Sign::CROSS) ? cross_open_four : circle_open_four);
						break;
					case ThreatType::FIVE:
						remove_single_threat(move, (sign == Sign::CROSS) ? cross_five : circle_five);
						break;
				}
			}
			template<Sign sign>
			void add_threat(ThreatType threat, Move move) noexcept
			{
				switch (threat)
				{
					default:
						break;
					case ThreatType::HALF_OPEN_FOUR:
						add_single_threat(move, (sign == Sign::CROSS) ? cross_half_open_four : circle_half_open_four);
						break;
					case ThreatType::OPEN_FOUR:
						add_single_threat(move, (sign == Sign::CROSS) ? cross_open_four : circle_open_four);
						break;
					case ThreatType::FIVE:
						add_single_threat(move, (sign == Sign::CROSS) ? cross_five : circle_five);
						break;
				}
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_V2_HPP_ */
