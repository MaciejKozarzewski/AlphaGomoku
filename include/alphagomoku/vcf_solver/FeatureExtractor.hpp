/*
 * FeatureExtractor.hpp
 *
 *  Created on: 2 maj 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_

#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>

#include <unordered_map>

namespace ag
{
	struct GameConfig;
} /* namespace ag */

namespace ag
{
	struct ThreatMove
	{
			ThreatType threat;
			Move move;
			ThreatMove(ThreatType t, Move m) :
					threat(t),
					move(m)
			{
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
			int max_positions = 1000;
			int position_counter = 0;
			int node_counter = 0;
			std::vector<InternalNode> nodes_buffer;

			GameRules rules;
			int pad;

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
			std::unordered_map<uint64_t, SolvedValue> hashtable;
		public:
			FeatureExtractor(GameConfig gameConfig);

			ProvenValue generateMoves(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList, const matrix<Sign> &board,
					Sign signToMove);

			void printFeature(int x, int y);
			void printRawFeature(int left, int right);
			void print();

			void setBoard(const matrix<Sign> &board);
			void addMove(Move move);
			void undoMove(Move move);
		private:
			uint32_t get_feature_at(int row, int col, Direction dir);
			void calc_all_features();
			void get_threat_lists();
			void solve(InternalNode &node, bool mustProveAllChildren, int depth);
			void update_threats(int row, int col);
			static std::string toString(SolvedValue sv);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_ */
