/*
 * EdgeGenerator.hpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <memory>
#include <vector>

namespace ag
{
	class Edge;
	class SearchTask;
}

namespace ag
{

	class EdgeGenerator
	{
		public:
			virtual ~EdgeGenerator() = default;
			virtual EdgeGenerator* clone() const = 0;
			virtual void generate(SearchTask &task) const = 0;
	};

	class BaseGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold;
			int max_edges;
		public:
			BaseGenerator();
			BaseGenerator(float policyThreshold, int maxEdges);
			BaseGenerator* clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that assumes that VCF solver was run on the given task.
	 * It is because VCF solver has the ability to check for game end conditions, eliminating the need to do it again during edge generation.
	 */
	class SolverGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold;
			int max_edges;
		public:
			SolverGenerator();
			SolverGenerator(float policyThreshold, int maxEdges);
			SolverGenerator* clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_ */
