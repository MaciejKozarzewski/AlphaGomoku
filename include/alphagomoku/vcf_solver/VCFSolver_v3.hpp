/*
 * VCFSolver_v3.hpp
 *
 *  Created on: Apr 13, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_V3_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_V3_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/vcf_solver/FastHashTable.hpp>
#include <alphagomoku/vcf_solver/VCFSolver.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor_v3.hpp>

namespace ag
{
	class SearchTask;
}

namespace ag
{
//	enum class SolvedValue : int8_t
//	{
//		UNKNOWN,
//		UNSOLVED,
//		LOSS,
//		WIN
//	};
//	inline std::string toString(SolvedValue sv)
//	{
//		switch (sv)
//		{
//			default:
//			case SolvedValue::UNKNOWN:
//				return "UNKNOWN";
//			case SolvedValue::UNSOLVED:
//				return "UNSOLVED";
//			case SolvedValue::LOSS:
//				return "LOSS";
//			case SolvedValue::WIN:
//				return "WIN";
//		}
//	}
//
//	struct SolverStats
//	{
//			TimedStat setup;
//			TimedStat static_solve;
//			TimedStat recursive_solve;
//
//			SolverStats();
//			std::string toString() const;
//	};

//	class Measurement
//	{
//			std::vector<std::pair<int, float>> m_values;
//			int m_param_value;
//		public:
//			Measurement(int paramValue) noexcept;
//			void clear() noexcept;
//			int getParamValue() const noexcept;
//			void update(int x, float y) noexcept;
//			std::pair<float, float> predict(int x) const noexcept;
//			std::string toString() const;
//	};

	class VCFSolver_v3
	{
		private:
			struct InternalNode
			{
					InternalNode *children = nullptr;
					Move move;
					int16_t number_of_children = 0;
					SolvedValue solved_value = SolvedValue::UNKNOWN;
					uint8_t prior_value = 0;
					void init(int row, int col, Sign sign, SolvedValue sv = SolvedValue::UNKNOWN) noexcept
					{
						children = nullptr;
						move = Move(row, col, sign);
						number_of_children = 0;
						solved_value = sv;
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

			FeatureExtractor_v3 feature_extractor;
			FastHashTable<uint32_t, SolvedValue, 4> hashtable;

			size_t step_counter = 0;
			int tuning_step = 2;
			Measurement lower_measurement;
			Measurement upper_measurement;

			SolverStats stats;
		public:
			VCFSolver_v3(GameConfig gameConfig, int maxPositions = 100);

			void solve(SearchTask &task, int level = 0);
			void tune(float speed);

			SolverStats getStats() const;
			void clearStats();

			void print_stats() const;
		private:
			void add_move(Move move) noexcept;
			void undo_move(Move move) noexcept;
			void check_game_end_conditions(SearchTask &task);
			void static_solve(SearchTask &task, int level);
			void prepare_moves_for_attacker(InternalNode &node);
			void prepare_moves_for_defender(InternalNode &node);
			void recursive_solve(InternalNode &node, bool isAttackingSide);

			const std::vector<Move>& get_attacker_threats(ThreatType_v3 tt) const noexcept;
			const std::vector<Move>& get_defender_threats(ThreatType_v3 tt) const noexcept;
			void add_children_nodes(InternalNode &node, const std::vector<Move> &moves, SolvedValue sv, bool sort = false) noexcept;
			void delete_children_nodes(InternalNode &node) noexcept;
	};

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_VCFSOLVER_V3_HPP_ */
