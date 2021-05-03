/*
 * FeatureExtractor.hpp
 *
 *  Created on: 2 maj 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_
#define ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_

#include <alphagomoku/rules/game_rules.hpp>
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

	class FeatureExtractor
	{
		private:
			GameRules rules;
			int pad;

			matrix<int> internal_board;
			matrix<uint32_t> features;

			std::vector<ThreatMove> cross_threats;
			std::vector<ThreatMove> circle_threats;
		public:
			FeatureExtractor(GameConfig gameConfig);

			void generateMoves(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList, const matrix<Sign> &board, Sign signToMove);

			void printFeature(int x, int y);
			void printRawFeature(int left, int right);
			void print();
		private:
			uint32_t get_feature_at(int row, int col, int dir);
			void calc_all_features();
			void get_threat_lists(const matrix<Sign> &board, Sign signToMove);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VCF_SOLVER_FEATUREEXTRACTOR_HPP_ */
