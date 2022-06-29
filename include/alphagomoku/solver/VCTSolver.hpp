/*
 * VCTSolver.hpp
 *
 *  Created on: Apr 21, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_VCTSOLVER_HPP_
#define ALPHAGOMOKU_SOLVER_VCTSOLVER_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/solver/PatternCalculator.hpp>
#include <alphagomoku/vcf_solver/FastHashTable.hpp>

#include <cassert>
#include <algorithm>
#include <iostream>

namespace ag
{
	class SearchTask;
}

namespace ag::experimental
{

	enum class SolvedValue : int8_t
	{
		UNCHECKED,
		FORBIDDEN,
		NO_SOLUTION,
		LOSS,
		DRAW,
		WIN
	};
	inline std::string toString(SolvedValue sv)
	{
		switch (sv)
		{
			default:
			case SolvedValue::UNCHECKED:
				return "UNCHECKED";
			case SolvedValue::NO_SOLUTION:
				return "NO_SOLUTION";
			case SolvedValue::LOSS:
				return "LOSS";
			case SolvedValue::WIN:
				return "WIN";
		}
	}

	struct SolverStats
	{
			TimedStat setup;
			TimedStat solve;
			int64_t hits = 0;
			int64_t total_positions = 0;

			SolverStats();
			std::string toString() const;
	};

	class Measurement
	{
			std::vector<std::pair<int, float>> m_values;
			int m_param_value;
		public:
			Measurement(int paramValue) noexcept;
			void clear() noexcept;
			int getParamValue() const noexcept;
			void update(int x, float y) noexcept;
			std::pair<float, float> predict(int x) const noexcept;
			std::string toString() const;
	};

	class VCTSolver
	{
		private:
			struct InternalNode
			{
					InternalNode *children = nullptr;
					Move move;
					int16_t number_of_children = 0;
					int8_t prior_value = 0;
					SolvedValue solved_value :6;
					bool must_defend :2;

					InternalNode() noexcept :
							children(nullptr),
							number_of_children(0),
							prior_value(0),
							solved_value(SolvedValue::UNCHECKED),
							must_defend(false)
					{
					}

					Sign getSignToMove() const noexcept
					{
						return invertSign(move.sign);
					}
					bool hasSolution() const noexcept
					{
						return solved_value > SolvedValue::NO_SOLUTION;
					}
					void init(int row, int col, Sign sign, SolvedValue sv = SolvedValue::UNCHECKED) noexcept
					{
						children = nullptr;
						move = Move(row, col, sign);
						number_of_children = 0;
						prior_value = 0;
						solved_value = sv;
						must_defend = false;
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
					bool containsChild(Move m) const noexcept
					{
						return std::any_of(children, children + number_of_children, [m](const InternalNode &node)
						{	return node.move.row == m.row and node.move.col == m.col;});
					}
					void print() const
					{
						std::cout << "Node after move " << move.toString() << " : " << move.text() << " : " << toString(solved_value) << "\n";
						std::cout << "must defend : " << must_defend << '\n' << "Children nodes:\n";
						for (int i = 0; i < number_of_children; i++)
							std::cout << i << " : " << children[i].move.toString() << " : " << children[i].move.text() << " = "
									<< toString(children[i].solved_value) << " " << (int) children[i].prior_value << '\n';
						std::cout << std::endl;
					}
			};

			std::vector<InternalNode> node_stack;
			size_t node_counter = 0;
			int number_of_moves_for_draw; // after that number of moves, a draw will be concluded
			int max_positions; // maximum number of positions that will be searched
			int depth = 0;
			Sign attacking_sign = Sign::NONE;

			double position_counter = 0.0;

			GameConfig game_config;
			PatternCalculator pattern_calculator;
			FastHashTable<uint32_t, SolvedValue, 4> hashtable;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;

			SolverStats stats;
		public:
			VCTSolver(GameConfig gameConfig, int maxPositions = 100);

			void solve(SearchTask &task, int level = 0);
			void tune(float speed);

			SolverStats getStats() const;
			void clearStats();
			void print_stats()
			{
				pattern_calculator.print_stats();
				std::cout << stats.toString() << '\n';
			}
		private:
			void get_forbidden_moves(SearchTask &task);
			bool try_win_in_1(InternalNode &node);
			bool try_draw_in_1(InternalNode &node);
			bool try_defend_opponent_five(InternalNode &node);
			bool try_win_in_3(InternalNode &node);
			bool try_solve_forks_if_opponent_has_no_fours(InternalNode &node);
			bool try_solve_4x3_forks(InternalNode &node);
			bool try_defend_opponent_fours(InternalNode &node);
			void generate_moves(InternalNode &node);

			void recursive_solve(InternalNode &node);

			const std::vector<Move>& get_own_threats(const InternalNode &node, ThreatType tt) const noexcept;
			const std::vector<Move>& get_opp_threats(const InternalNode &node, ThreatType tt) const noexcept;
			void add_defensive_moves(InternalNode &node, const Move move, Direction dir);
			void add_child_node(InternalNode &node, Move move, SolvedValue sv = SolvedValue::UNCHECKED, bool excludeDuplicates = false) noexcept;
			void add_children_nodes(InternalNode &node, const std::vector<Move> &moves, SolvedValue sv = SolvedValue::UNCHECKED,
					bool excludeDuplicates = false) noexcept;
			void delete_children_nodes(InternalNode &node) noexcept;
			void calculate_solved_value(InternalNode &node) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_VCTSOLVER_HPP_ */
