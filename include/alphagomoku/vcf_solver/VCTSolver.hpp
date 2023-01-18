/*
 * VCTSolver_v3.hpp
 *
 *  Created on: Apr 13, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCTSOLVER_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCTSOLVER_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/vcf_solver/FastHashTable.hpp>
#include <alphagomoku/vcf_solver/VCFSolver.hpp>

#include <cassert>

namespace ag
{
	class SearchTask;
}

namespace ag::solver
{

	class VCTSolver
	{
		private:
			struct InternalNode
			{
					InternalNode *children = nullptr;
					Move move;
					int16_t number_of_children = 0;
					SolvedValue solved_value = SolvedValue::UNKNOWN;
					int8_t prior_value = 0;
					void init(int row, int col, Sign sign, int8_t prior = 0) noexcept
					{
						children = nullptr;
						move = Move(row, col, sign);
						number_of_children = 0;
						solved_value = SolvedValue::UNKNOWN;
						prior_value = prior;
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

			int number_of_moves_for_draw; // after that number of moves, a draw will be concluded
			int max_positions; // maximum number of positions that will be searched

			double position_counter = 0.0;
			size_t node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameConfig game_config;

			PatternCalculator feature_extractor;
			FastHashTable<uint32_t, SolvedValue, 4> hashtable;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;

			SolverStats stats;

			matrix<int16_t> hint_table;
		public:
			VCTSolver(GameConfig gameConfig, int maxPositions = 100);

			void solve(SearchTask &task, int level = 0);
			void tune(float speed);

			SolverStats getStats() const;
			void clearStats();

			void print_stats() const;
		private:
			void check_game_end_conditions(SearchTask &task);
			void static_solve(SearchTask &task, int level);
			void prepare_moves_for_attacker(InternalNode &node);
			void prepare_moves_for_defender(InternalNode &node);
			void recursive_solve(InternalNode &node, bool isAttackingSide);

			const std::vector<Location>& get_own_threats(ag::ThreatType tt) const noexcept;
			const std::vector<Location>& get_opponent_threats(ag::ThreatType tt) const noexcept;
			void add_children_nodes(InternalNode &node, const std::vector<Location> &moves, int8_t prior = 0) noexcept;
			void delete_children_nodes(InternalNode &node) noexcept;
			void update_hint_table(const InternalNode &node) noexcept;
			int get_node_value(const InternalNode &node) const noexcept;
	};

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCTSOLVER_HPP_ */
