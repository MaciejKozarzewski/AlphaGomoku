/*
 * FastPolicy.hpp
 *
 *  Created on: Apr 16, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_

#include <alphagomoku/vcf_solver/FeatureExtractor_v3.hpp>

namespace ag
{

	class FastPolicy
	{
		private:
			FeatureExtractor_v3 feature_extractor;
			std::vector<float> feature_weights;
			std::vector<float> threat_weights;
			matrix<float> bias;
		public:
			FastPolicy(GameConfig gameConfig);

			std::vector<float> getAccuracy(int top_k = 4) const;

			void learn(const matrix<Sign> &board, Sign signToMove, const matrix<float> &target);
	};
} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_ */
