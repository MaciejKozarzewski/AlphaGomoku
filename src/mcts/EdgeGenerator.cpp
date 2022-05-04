/*
 * EdgeGenerator.cpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

#include <cassert>

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
			if (sum_priors == 0.0f)
			{
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
	void exclude_weak_moves(std::vector<Edge> &edges, size_t max_edges) noexcept
	{
		if (edges.size() <= max_edges)
			return;

		std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), [](const Edge &lhs, const Edge &rhs)
		{	return lhs.getPolicyPrior() > rhs.getPolicyPrior();});
		edges.erase(edges.begin() + max_edges, edges.end());
	}

	bool contains_winning_edge(const std::vector<Edge> &edges) noexcept
	{
		return std::any_of(edges.begin(), edges.end(), [](const Edge &edge)
		{	return edge.getProvenValue() == ProvenValue::WIN;});
	}
	void check_terminal_conditions(SearchTask &task, Edge &edge)
	{
		if (not edge.isProven())
		{
			Move move = edge.getMove();

			Board::putMove(task.getBoard(), move);
			GameOutcome outcome = Board::getOutcome(task.getGameRules(), task.getBoard(), move);
			Board::undoMove(task.getBoard(), move);

			edge.setProvenValue(convertProvenValue(outcome, task.getSignToMove()));
		}
	}
	void assign_policy_and_Q(SearchTask &task, Edge &edge) noexcept
	{
		switch (edge.getProvenValue())
		{
			case ProvenValue::UNKNOWN:
			{
				Move move = edge.getMove();
				edge.setPolicyPrior(task.getPolicy().at(move.row, move.col));
				edge.setValue(task.getActionValues().at(move.row, move.col));
				break;
			}
			case ProvenValue::LOSS:
			{
				edge.setPolicyPrior(1.0e-6f); // setting zero would crash renormalization in case when all moves are provably losing as the policy sum would be 0
				edge.setValue(Value(0.0f, 0.0f, 1.0f));
				break;
			}
			case ProvenValue::DRAW:
			{
				Move move = edge.getMove();
				edge.setPolicyPrior(task.getPolicy().at(move.row, move.col));
				edge.setValue(Value(0.0f, 1.0f, 0.0f));
				break;
			}
			case ProvenValue::WIN:
			{
				edge.setPolicyPrior(1.0e3f);
				edge.setValue(Value(1.0f, 0.0f, 0.0f));
				break;
			}
		}
	}
}

namespace ag
{
	BaseGenerator::BaseGenerator() :
			policy_threshold(0.0f),
			max_edges(std::numeric_limits<int>::max())
	{
	}
	BaseGenerator::BaseGenerator(float policyThreshold, int maxEdges) :
			policy_threshold(policyThreshold),
			max_edges(maxEdges)
	{
	}
	BaseGenerator* BaseGenerator::clone() const
	{
		return new BaseGenerator(policy_threshold, max_edges);
	}
	void BaseGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getProvenEdges().size() > 0)
		{
			for (size_t i = 0; i < task.getProvenEdges().size(); i++)
				task.addEdge(task.getProvenEdges()[i]);
		}
		else
		{
			if (not contains_winning_edge(task.getEdges()))
			{
				for (int row = 0; row < task.getBoard().rows(); row++)
					for (int col = 0; col < task.getBoard().cols(); col++)
						if (task.getBoard().at(row, col) == Sign::NONE and task.getPolicy().at(row, col) >= policy_threshold)
							task.addEdge(Move(row, col, task.getSignToMove()));
			}
		}

		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
		{
			check_terminal_conditions(task, *edge);
			assign_policy_and_Q(task, *edge);
		}

		exclude_weak_moves(task.getEdges(), max_edges);

		renormalize_edges(task.getEdges());
		assert(task.getEdges().size() > 0);
	}

	SolverGenerator::SolverGenerator() :
			policy_threshold(0.0f),
			max_edges(std::numeric_limits<int>::max())
	{
	}
	SolverGenerator::SolverGenerator(float policyThreshold, int maxEdges) :
			policy_threshold(policyThreshold),
			max_edges(maxEdges)
	{
	}
	SolverGenerator* SolverGenerator::clone() const
	{
		return new SolverGenerator(policy_threshold, max_edges);
	}
	void SolverGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getNonLosingEdges().size() > 0)
		{
			for (size_t i = 0; i < task.getNonLosingEdges().size(); i++)
				task.addEdge(task.getNonLosingEdges()[i]);
		}
		else
		{
			if (not contains_winning_edge(task.getEdges()))
			{
				for (int row = 0; row < task.getBoard().rows(); row++)
					for (int col = 0; col < task.getBoard().cols(); col++)
						if (task.getBoard().at(row, col) == Sign::NONE and task.getPolicy().at(row, col) >= policy_threshold)
							task.addEdge(Move(row, col, task.getSignToMove()));
			}
		}

		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			assign_policy_and_Q(task, *edge);

		exclude_weak_moves(task.getEdges(), max_edges);

		renormalize_edges(task.getEdges());
		assert(task.getEdges().size() > 0);
	}

	NoisyGenerator::NoisyGenerator(const matrix<float> &noiseMatrix, float noiseWeight, const EdgeGenerator &baseGenerator) :
			noise_matrix(noiseMatrix),
			noise_weight(noiseWeight),
			base_generator(std::unique_ptr<EdgeGenerator>(baseGenerator.clone()))
	{
	}
	NoisyGenerator* NoisyGenerator::clone() const
	{
		return new NoisyGenerator(noise_matrix, noise_weight, *base_generator);
	}
	void NoisyGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.visitedPathLength() == 0)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						task.addEdge(Move(row, col, task.getSignToMove()));

			for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			{
				check_terminal_conditions(task, *edge);
				assign_policy_and_Q(task, *edge);
				Move m = edge->getMove();
				edge->setPolicyPrior((1.0f - noise_weight) * edge->getPolicyPrior() + noise_weight * noise_matrix.at(m.row, m.col));
			}
			renormalize_edges(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);
	}

	BalancedGenerator::BalancedGenerator(int balanceDepth, const EdgeGenerator &baseGenerator) :
			balance_depth(balanceDepth),
			base_generator(std::unique_ptr<EdgeGenerator>(baseGenerator.clone()))
	{
	}
	BalancedGenerator* BalancedGenerator::clone() const
	{
		return new BalancedGenerator(balance_depth, *base_generator);
	}
	void BalancedGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getLastPair().node->getDepth() < balance_depth)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						task.addEdge(Move(row, col, task.getSignToMove()));

			for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			{
				check_terminal_conditions(task, *edge);
				assign_policy_and_Q(task, *edge);
			}

			renormalize_edges(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);

	}

} /* namespace ag */

