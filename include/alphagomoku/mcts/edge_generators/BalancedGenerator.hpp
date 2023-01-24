/*
 * BalancedGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BALANCEDGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BALANCEDGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that adds all edges.
	 */
	class BalancedGenerator: public EdgeGenerator
	{
		private:
			const int balance_depth;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			BalancedGenerator(int balanceDepth, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_BALANCEDGENERATOR_HPP_ */
