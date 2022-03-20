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

	struct SolverStats
	{
			TimedStat setup;
			TimedStat static_solve;
			TimedStat recursive_solve;

			SolverStats();
			std::string toString() const;
	};

	class AutoTuner
	{
		private:
			struct Stat
			{
					float value = 0.0f;
					int visits = 0;
			};
			std::vector<int> m_positions;
			std::vector<Stat> m_stats;
		public:
			AutoTuner(int positions);
			void reset() noexcept;
			void update(int positions, float speed) noexcept;
			int select() const noexcept;
	};

	class VCFSolver
	{
		private:
			struct InternalNode
			{
					InternalNode *children = nullptr;
					Move move;
					int16_t number_of_children = 0;
					SolvedValue solved_value = SolvedValue::UNKNOWN;
					void init(int row, int col, Sign sign) noexcept
					{
						children = nullptr;
						move = Move(row, col, sign);
						number_of_children = 0;
						solved_value = SolvedValue::UNKNOWN;
					}
			};

			int max_positions = 100; // maximum number of positions that will be searched
			bool use_caching = true; // whether to use position caching or not

			int position_counter = 0;
			int node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;

			FeatureExtractor_v2 feature_extractor;
			FastHashTable<uint32_t, SolvedValue, 4> hashtable;

			AutoTuner automatic_tuner;

			SolverStats stats;
		public:
			VCFSolver(GameConfig gameConfig, int maxPositions = 100);

			void solve(SearchTask &task, int level = 0);
			void tune(float speed);

			SolverStats getStats() const;
			void clearStats();
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
