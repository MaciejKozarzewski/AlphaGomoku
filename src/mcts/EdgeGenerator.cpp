/*
 * EdgeGenerator.cpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

namespace
{
	using namespace ag;

	void renormalize_edges(std::vector<Edge> &edges) noexcept
	{
		float sum_priors = 0.0f;
		for (size_t i = 0; i < edges.size(); i++)
			sum_priors += edges[i].getPolicyPrior();
		if (fabsf(sum_priors - 1.0f) > 0.01f) // do not renormalize if the sum is close to 1
		{
			assert(sum_priors > 0.0f);
			sum_priors = 1.0f / sum_priors;
			for (size_t i = 0; i < edges.size(); i++)
				edges[i].setPolicyPrior(edges[i].getPolicyPrior() * sum_priors);
		}
	}
	void exclude_weak_moves(std::vector<Edge> &edges, size_t max_edges) noexcept
	{
		std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), [](const Edge &lhs, const Edge &rhs)
		{	return lhs.getPolicyPrior() > rhs.getPolicyPrior();});
		edges.erase(edges.begin() + max_edges, edges.end());
	}

	bool contains_winning_edge(const std::vector<Edge> &edges) noexcept
	{
		for (size_t i = 0; i < edges.size(); i++)
			if (edges[i].getProvenValue() == ProvenValue::WIN)
				return true;
		return false;
	}
}

namespace ag
{
	EdgeGenerator::EdgeGenerator(GameRules rules, float policyThreshold, int maxEdges, ExclusionConfig config) :
			game_rules(rules),
			policy_threshold(policyThreshold),
			max_edges(maxEdges)
	{
	}
	void EdgeGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getProvenEdges().size() > 0)
		{
			for (size_t i = 0; i < task.getProvenEdges().size(); i++)
				task.addEdge(task.getProvenEdges()[i]);
		}

		if (not contains_winning_edge(task.getEdges()))
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE and task.getPolicy().at(row, col) >= policy_threshold)
						task.addEdge(Move(row, col, task.getSignToMove()));
		}

		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
		{
			if (not edge->isProven())
			{
				Move move = edge->getMove();

				Board::putMove(task.getBoard(), move);
				GameOutcome outcome = Board::getOutcome(game_rules, task.getBoard(), move);
				Board::undoMove(task.getBoard(), move);

				edge->setProvenValue(convertProvenValue(outcome, task.getSignToMove()));
				edge->setPolicyPrior(task.getPolicy().at(move.row, move.col));
				edge->setValue(task.getActionValues().at(move.row, move.col));
			}
			switch (edge->getProvenValue())
			{
				case ProvenValue::UNKNOWN:
					break;
				case ProvenValue::LOSS:
				{
					edge->setPolicyPrior(0.0f);
					edge->setValue(Value(0.0f, 0.0f, 1.0f));
					break;
				}
				case ProvenValue::DRAW:
				{
					edge->setValue(Value(0.0f, 1.0f, 0.0f));
					break;
				}
				case ProvenValue::WIN:
				{
					edge->setPolicyPrior(1.0e3f);
					edge->setValue(Value(1.0f, 0.0f, 0.0f));
					break;
				}
			}
		}

		if (static_cast<int>(task.getEdges().size()) > max_edges)
			exclude_weak_moves(task.getEdges(), max_edges);

		renormalize_edges(task.getEdges());
	}

} /* namespace ag */

