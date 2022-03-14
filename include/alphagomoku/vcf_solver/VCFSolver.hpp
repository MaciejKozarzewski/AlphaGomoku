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
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/vcf_solver/FastHashTable.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor_v2.hpp>

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

			int max_positions = 100; // maximum number of positions that will be searched
			bool use_caching = true; // whether to use position caching or not

			int position_counter = 0;
			int node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;

			FeatureExtractor_v2 feature_extractor;
			FastHashTable<uint32_t, SolvedValue> hashtable;

			TimedStat time_setup;
			TimedStat time_static;
			TimedStat time_recursive;

			struct TuningPoint
			{
					int positions = 0;
					float speed = 0.0f;
			};
			TuningPoint last_tuning_points;
			double tuning_step = 1.1;
		public:
			VCFSolver(GameConfig gameConfig, int maxPositions = 100);

			void solve(SearchTask &task, int level = 0);

			void tune(float speed);
		private:
			void add_move(Move move) noexcept;
			void undo_move(Move move) noexcept;
			bool static_solve_1ply_win(SearchTask &task);
			bool static_solve_1ply_draw(SearchTask &task);
			bool static_solve_block_5(SearchTask &task);
			bool static_solve_3ply_win(SearchTask &task);
			bool static_solve_block_4(SearchTask &task);

			void recursive_solve(InternalNode &node, bool mustProveAllChildren);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_HPP_ */
