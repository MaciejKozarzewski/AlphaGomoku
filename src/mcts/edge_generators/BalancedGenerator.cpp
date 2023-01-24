/*
 * BalancedGenerator.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_generators/BalancedGenerator.hpp>
#include <alphagomoku/mcts/edge_generators/generator_utils.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/augmentations.hpp>

#include <cassert>
#include <iostream>

namespace ag
{

	BalancedGenerator::BalancedGenerator(int balanceDepth, const EdgeGenerator &baseGenerator) :
			balance_depth(balanceDepth),
			base_generator(baseGenerator.clone())
	{
	}
	std::unique_ptr<EdgeGenerator> BalancedGenerator::clone() const
	{
		return std::make_unique<BalancedGenerator>(balance_depth, *base_generator);
	}
	void BalancedGenerator::generate(SearchTask &task) const
	{
		assert(task.isReadyNetwork() or task.isReadySolver());

		if (task.getAbsoluteDepth() < balance_depth)
		{
			create_moves_from_policy(task, 0.0f);

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
