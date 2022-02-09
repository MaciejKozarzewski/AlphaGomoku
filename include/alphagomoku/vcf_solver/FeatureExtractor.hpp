/*
 * FeatureExtractor.hpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

#include <unordered_map>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	struct KeyHash
	{
			size_t operator()(uint64_t h) const noexcept
			{
				return static_cast<size_t>(h);
			}
	};
	struct KeyCompare
	{
			bool operator()(uint64_t lhs, uint64_t rhs) const noexcept
			{
				return lhs == rhs;
			}
	};

	enum class Direction
	{
		HORIZONTAL,
		VERTICAL,
		DIAGONAL,
		ANTIDIAGONAL
	};

	class FeatureExtractor
	{
		private:
			enum class SolvedValue
			{
				UNKNOWN,
				UNSOLVED,
				SOLVED_LOSS,
				SOLVED_WIN
			};
			struct InternalNode
			{
					InternalNode *children = nullptr;
					Move move;
					SolvedValue solved_value = SolvedValue::UNKNOWN;
					int number_of_children = 0;
					void init(int row, int col, Sign sign) noexcept
					{
						children = nullptr;
						move = Move(row, col, sign);
						solved_value = SolvedValue::UNKNOWN;
						number_of_children = 0;
					}
			};
			struct solver_stats
			{
					int calls = 0;
					int hits = 0;
					int positions_hit = 0;
					int positions_miss = 0;
					void add(bool is_hit, int positions_checked) noexcept
					{
						calls++;
						if (is_hit)
						{
							hits++;
							positions_hit += positions_checked;
						}
						else
							positions_miss += positions_checked;
					}
			};
			std::vector<solver_stats> statistics;

			int total_positions = 0;
			int root_depth = 0;

			int max_positions = 100; // maximum number of positions that will be searched
			int max_depth = 30; // maximum recursion depth
			bool use_caching = true; // whether to use position caching or not
			int cache_size = 1000; // number of positions in the cache

			int position_counter = 0;
			int node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;
			int pad;

			Sign sign_to_move = Sign::NONE;
			matrix<int> internal_board;
			matrix<uint32_t> features;

			std::vector<Move> cross_five;
			std::vector<Move> cross_open_four;
			std::vector<Move> cross_half_open_four;

			std::vector<Move> circle_five;
			std::vector<Move> circle_open_four;
			std::vector<Move> circle_half_open_four;

			matrix<ThreatType> cross_threats;
			matrix<ThreatType> circle_threats;

			std::unordered_map<uint64_t, SolvedValue, KeyHash, KeyCompare> hashtable;
			std::vector<uint64_t> hashing_keys;
			uint64_t current_board_hash = 0;
		public:
			FeatureExtractor(GameConfig gameConfig);

			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void solve(SearchTask &task, int level = 0);
			ProvenValue solve(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList);

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
			uint64_t get_hash() const noexcept;
			uint64_t update_hash(uint64_t old_hash, Move move) const noexcept;
			void create_hashtable();
			uint32_t get_feature_at(int row, int col, Direction dir) const noexcept;
			void calc_all_features() noexcept;
			void get_threat_lists();
			void recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth);
			static std::string toString(SolvedValue sv);

			void update_threats(int row, int col);
			void update_threat_at(const FeatureTable &table, int row, int col, Direction direction);
			void update_threat_list(ThreatType old_threat, ThreatType new_threat, Move move, std::vector<Move> &five, std::vector<Move> &open_four,
					std::vector<Move> &half_open_four);
			ThreatType get_best_threat_at(const matrix<ThreatType> &threats, int row, int col) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_ */
