/*
 * CenterOnlyGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTERONLYGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTERONLYGENERATOR_HPP_

#include <alphagomoku/mcts/edge_generators/EdgeGenerator.hpp>

#include <memory>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	/**
	 * \brief Generator that generates balanced moves at depth 0 including only central NxN square
	 */
	class CenterOnlyGenerator: public EdgeGenerator
	{
		private:
			const int square_size;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			CenterOnlyGenerator(int squareSize, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_CENTERONLYGENERATOR_HPP_ */
