/*
 * VCFSolver.hpp
 *
 *  Created on: Mar 3, 2022
 *      Author: Maciej Kozarzeski
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/vcf_solver/FastHashTable.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor_v2.hpp>

#include <unordered_map>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	enum class SolvedValue : uint8_t
	{
		UNKNOWN,
		UNSOLVED,
		LOSS,
		WIN
	};

	class VCFSolver
	{
		private:
			struct MoveValuePair
			{
					Move move;
					SolvedValue solved_value;
			};

			/**
			 * \brief Structure representing single level of search.
			 */
			struct SolverData
			{
				private:
					int losing_actions_count = 0;
					bool has_win_action = false;
					bool has_unsolved_action = false;
				public:
					std::vector<MoveValuePair> actions;
					size_t current_action_index = 0;
					SolvedValue solved_value = SolvedValue::UNKNOWN;

					void clear() noexcept;
					void updateWith(SolvedValue sv) noexcept;
					SolvedValue getSolvedValue() const noexcept;
					bool isSolved() const noexcept;
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

			int max_positions = 1000; // maximum number of positions that will be searched
			int max_depth = 50; // maximum recursion depth
			bool use_caching = true; // whether to use position caching or not

			int position_counter = 0;
			int node_counter = 0;
			std::vector<SolverData> search_path;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;

			FeatureExtractor_v2 feature_extractor;
			FastHashTable<uint32_t, SolvedValue> hashtable;

		public:
			VCFSolver(GameConfig gameConfig);

			void solve(SearchTask &task, int level = 0);

			void print_stats() const;
		private:
			void add_move(Move move) noexcept;
			void undo_move(Move move) noexcept;
			bool static_solve_1ply_win(SearchTask &task);
			bool static_solve_1ply_draw(SearchTask &task);
			bool static_solve_block_5(SearchTask &task);
			bool static_solve_3ply_win(SearchTask &task);
			bool static_solve_block_4(SearchTask &task);

			void recursive_solve();
			void recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_HPP_ */
