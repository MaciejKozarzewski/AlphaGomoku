/*
 * generator_utils.hpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_GENERATORS_GENERATOR_UTILS_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_GENERATORS_GENERATOR_UTILS_HPP_

#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/game/rules.hpp>

namespace ag
{
	[[maybe_unused]] static void renormalize_policy(std::vector<Edge> &edges) noexcept
	{
		float sum_priors = 0.0f;
		for (size_t i = 0; i < edges.size(); i++)
			sum_priors += edges[i].getPolicyPrior();
		if (fabsf(sum_priors - 1.0f) > 0.01f) // do not renormalize if the sum is close to 1
		{
			if (sum_priors == 0.0f)
			{ // in this case divide the distribution evenly among all edges
				sum_priors = 1.0f / edges.size();
				for (size_t i = 0; i < edges.size(); i++)
					edges[i].setPolicyPrior(sum_priors);
			}
			else
			{
				sum_priors = 1.0f / sum_priors;
				for (size_t i = 0; i < edges.size(); i++)
					edges[i].setPolicyPrior(edges[i].getPolicyPrior() * sum_priors);
			}
		}
	}
	[[maybe_unused]] static void prune_low_policy_moves(std::vector<Edge> &edges, size_t max_edges) noexcept
	{
		if (edges.size() <= max_edges)
			return;

		std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), [](const Edge &lhs, const Edge &rhs)
		{	return lhs.getPolicyPrior() > rhs.getPolicyPrior();}); // sort by prior policy in descending order
		edges.erase(edges.begin() + max_edges, edges.end());
	}

	[[maybe_unused]] static void create_moves_from_solver(SearchTask &task) noexcept
	{
		switch (task.getProvenValue())
		{
			case ProvenValue::UNKNOWN:
			case ProvenValue::DRAW:
				for (auto edge = task.getPriorEdges().begin(); edge < task.getPriorEdges().end(); edge++)
					if (edge->getProvenValue() != ProvenValue::LOSS) // no need to use memory by adding losing edges
						task.addEdge(*edge);
				break;
			case ProvenValue::LOSS:
				for (auto edge = task.getPriorEdges().begin(); edge < task.getPriorEdges().end(); edge++)
					task.addEdge(*edge); // we must add all losing moves
				break;
			case ProvenValue::WIN:
				for (auto edge = task.getPriorEdges().begin(); edge < task.getPriorEdges().end(); edge++)
					if (edge->getProvenValue() == ProvenValue::WIN) // we just need winning edges here
						task.addEdge(*edge);
				break;
		}
	}
	[[maybe_unused]] static void create_moves_from_policy(SearchTask &task, float policyThreshold) noexcept
	{
		for (int row = 0; row < task.getBoard().rows(); row++)
			for (int col = 0; col < task.getBoard().cols(); col++)
				if (task.getBoard().at(row, col) == Sign::NONE and task.getPolicy().at(row, col) >= policyThreshold)
				{
					const Move move(row, col, task.getSignToMove());
					const float policy = task.getPolicy().at(row, col);
					const Value action_value = task.getActionValues().at(row, col);
					task.addEdge(move, policy, action_value);
				}
	}
	[[maybe_unused]] static void check_terminal_conditions(SearchTask &task, Edge &edge)
	{
		if (not edge.isProven())
		{
			const Move move = edge.getMove();

			Board::putMove(task.getBoard(), move);
			const GameOutcome outcome = getOutcome_v2(task.getGameRules(), task.getBoard(), move);
			Board::undoMove(task.getBoard(), move);

			edge.setProvenValue(convertProvenValue(outcome, task.getSignToMove()));
		}
	}
	[[maybe_unused]] static void override_proven_move_policy_and_value(const SearchTask &task, Edge &edge) noexcept
	{
		const Move move = edge.getMove();
		switch (edge.getProvenValue())
		{
			case ProvenValue::UNKNOWN:
				edge.setPolicyPrior(task.getPolicy().at(move.row, move.col));
				break;
			case ProvenValue::LOSS:
				edge.setPolicyPrior(0.0f);
				edge.setValue(Value(0.0f, 0.0f, 1.0f));
				break;
			case ProvenValue::DRAW:
				edge.setPolicyPrior(task.getPolicy().at(move.row, move.col));
				edge.setValue(Value(0.0f, 1.0f, 0.0f));
				break;
			case ProvenValue::WIN:
				edge.setPolicyPrior(1.0e3f);
				edge.setValue(Value(1.0f, 0.0f, 0.0f));
				break;
		}
	}
	[[maybe_unused]] static bool is_in_center_square(const SearchTask &task, int size, int row, int col)
	{
		const int top = task.getBoard().rows() / 2 - size / 2;
		const int left = task.getBoard().cols() / 2 - size / 2;
		return row >= top and row < top + size and col >= left and col < left + size;
	}
}

#endif /* ALPHAGOMOKU_MCTS_EDGE_GENERATORS_GENERATOR_UTILS_HPP_ */
