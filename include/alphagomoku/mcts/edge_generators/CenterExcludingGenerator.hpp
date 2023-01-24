/*
 * CenterExcludingGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTEREXCLUDINGGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTEREXCLUDINGGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that generates balanced moves at depth 0 excluding central NxN square
	 */
	class CenterExcludingGenerator: public EdgeGenerator
	{
		private:
			const int square_size;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			CenterExcludingGenerator(int squareSize, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTEREXCLUDINGGENERATOR_HPP_ */
