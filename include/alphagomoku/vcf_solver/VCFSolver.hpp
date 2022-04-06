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
	inline std::string toString(SolvedValue sv)
	{
		switch (sv)
		{
			default:
			case SolvedValue::UNKNOWN:
				return "UNKNOWN";
			case SolvedValue::UNSOLVED:
				return "UNSOLVED";
			case SolvedValue::LOSS:
				return "LOSS";
			case SolvedValue::WIN:
				return "WIN";
		}
	}

	struct SolverStats
	{
			TimedStat setup;
			TimedStat static_solve;
			TimedStat recursive_solve;

			SolverStats();
			std::string toString() const;
	};

	class Measurement
	{
			std::vector<std::pair<int, float>> m_values;
			int m_param_value;
		public:
			Measurement(int paramValue) noexcept;
			int getParamValue() const noexcept;
			void update(int x, float y) noexcept;
			std::pair<float, float> predict(int x) const noexcept;
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
					InternalNode* begin() noexcept
					{
						assert(children != nullptr);
						return children;
					}
					InternalNode* end() noexcept
					{
						assert(children != nullptr);
						return children + number_of_children;
					}
			};

			int max_positions; // maximum number of positions that will be searched

			double position_counter = 0.0;
			int node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;

			FeatureExtractor_v2 feature_extractor;
			FastHashTable<uint32_t, SolvedValue, 4> hashtable;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;

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

			void recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth = 0);
			void recursive_solve_2(InternalNode &node, bool isAttackingSide);

			const std::vector<Move>& get_attacker_threats(ThreatType tt) noexcept;
			const std::vector<Move>& get_defender_threats(ThreatType tt) noexcept;
			void create_node_stack(InternalNode &node, int numberOfNodes) noexcept;
			void delete_node_stack(InternalNode &node) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_HPP_ */
