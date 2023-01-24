/*
 * BaseGenerator.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_generators/BaseGenerator.hpp>
#include <alphagomoku/mcts/edge_generators/generator_utils.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/augmentations.hpp>

#include <cassert>
#include <iostream>

namespace ag
{
	BaseGenerator::BaseGenerator(float policyThreshold, int maxEdges) :
			policy_threshold(policyThreshold),
			max_edges(maxEdges)
	{
	}
	std::unique_ptr<EdgeGenerator> BaseGenerator::clone() const
	{
		return std::make_unique<BaseGenerator>(policy_threshold, max_edges);
	}
	void BaseGenerator::generate(SearchTask &task) const
	{
		assert(task.isReadyNetwork() or task.isReadySolver());

		create_moves_from_policy(task, policy_threshold);

		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
		{
			check_terminal_conditions(task, *edge);
			override_proven_move_policy_and_value(task, *edge);
		}
		prune_low_policy_moves(task.getEdges(), max_edges);
		renormalize_policy(task.getEdges());
		assert(task.getEdges().size() > 0);
	}

} /* namespace ag */
