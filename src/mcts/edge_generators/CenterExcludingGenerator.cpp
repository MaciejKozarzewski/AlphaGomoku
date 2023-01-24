/*
 * CenterExcludingGenerator.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_generators/CenterExcludingGenerator.hpp>
#include <alphagomoku/mcts/edge_generators/generator_utils.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

#include <cassert>

namespace ag
{

	CenterExcludingGenerator::CenterExcludingGenerator(int squareSize, const EdgeGenerator &baseGenerator) :
			square_size(squareSize),
			base_generator(baseGenerator.clone())
	{
	}
	std::unique_ptr<EdgeGenerator> CenterExcludingGenerator::clone() const
	{
		return std::make_unique<CenterExcludingGenerator>(square_size, *base_generator);
	}
	void CenterExcludingGenerator::generate(SearchTask &task) const
	{
		assert(task.isReadyNetwork() or task.isReadySolver());

		if (task.getRelativeDepth() == 0)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE and not is_in_center_square(task, square_size, row, col))
						task.addEdge(Move(row, col, task.getSignToMove()));

			for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			{
				check_terminal_conditions(task, *edge);
				override_proven_move_policy_and_value(task, *edge);
			}

			renormalize_policy(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);
	}

} /* namespace ag */
