/*
 * SymmetricalExcludingGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SYMMETRICALEXCLUDINGGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SYMMETRICALEXCLUDINGGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>

namespace ag
{
	class Edge;
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that generates balanced moves at depth 0 excluding symmetrical moves (if they exist)
	 */
	class SymmetricalExcludingGenerator: public EdgeGenerator
	{
		private:
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			SymmetricalExcludingGenerator(const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_SYMMETRICALEXCLUDINGGENERATOR_HPP_ */
