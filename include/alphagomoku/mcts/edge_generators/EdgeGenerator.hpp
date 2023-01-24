/*
 * EdgeGenerator.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_EDGEGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_EDGEGENERATOR_HPP_

#include <memory>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	class EdgeGenerator
	{
		public:
			EdgeGenerator() = default;
			EdgeGenerator(const EdgeGenerator &other) = delete;
			EdgeGenerator(EdgeGenerator &&other) = delete;
			EdgeGenerator& operator=(const EdgeGenerator &other) = delete;
			EdgeGenerator& operator=(EdgeGenerator &&other) = delete;
			virtual ~EdgeGenerator() = default;
			virtual std::unique_ptr<EdgeGenerator> clone() const = 0;
			virtual void generate(SearchTask &task) const = 0;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_EDGEGENERATOR_HPP_ */
