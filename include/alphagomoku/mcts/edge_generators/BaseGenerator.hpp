/*
 * BaseGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BASEGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BASEGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>
#include <limits>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	class BaseGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold = 0.0f;
			int max_edges = std::numeric_limits<int>::max();
		public:
			BaseGenerator() = default;
			BaseGenerator(float policyThreshold, int maxEdges);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BASEGENERATOR_HPP_ */
