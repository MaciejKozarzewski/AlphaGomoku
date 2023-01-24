/*
 * SolverGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SOLVERGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SOLVERGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>
#include <limits>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that assumes that VCF solver was run on the given task.
	 * It is because VCF solver has the ability to check for game end conditions, eliminating the need to do it again during edge generation.
	 */
	class SolverGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold = 0.0f;
			int max_edges = std::numeric_limits<int>::max();
		public:
			SolverGenerator() = default;
			SolverGenerator(float policyThreshold, int maxEdges);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SOLVERGENERATOR_HPP_ */
