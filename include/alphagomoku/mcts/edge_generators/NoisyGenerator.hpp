/*
 * NoisyGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_NOISYGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_NOISYGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <memory>

namespace ag
{
	class Edge;
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that adds all edges and applies some noise to the policy.
	 */
	class NoisyGenerator: public EdgeGenerator
	{
		private:
			matrix<float> noise_matrix;
			float noise_weight;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			NoisyGenerator(const matrix<float> &noiseMatrix, float noiseWeight, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_NOISYGENERATOR_HPP_ */
