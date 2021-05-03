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
			GameRules rules;
			int pad;

			matrix<int> internal_board;
			matrix<uint32_t> features;

			std::vector<ThreatMove> cross_threats;
			std::vector<ThreatMove> circle_threats;
			matrix<Threat> map_of_threats;
		public:
			FeatureExtractor(GameConfig gameConfig);

			ProvenValue generateMoves(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList, const matrix<Sign> &board,
					Sign signToMove);

			void printFeature(int x, int y);
			void printRawFeature(int left, int right);
			void print();

		private:
			uint32_t get_feature_at(int row, int col, Direction dir);
			Threat get_threat_at(int row, int col, Direction dir);
			void calc_all_features();
			void get_threat_lists(Sign signToMove);
			ProvenValue solve(Sign signToMove);
			void update_threats(int row, int col);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_ */
