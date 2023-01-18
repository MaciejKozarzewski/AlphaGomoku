/*
 * EdgeGenerator.cpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/augmentations.hpp>

#include <cassert>
#include <iostream>

namespace
{
	using namespace ag;

	void renormalize_policy(std::vector<Edge> &edges) noexcept
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
	void prune_low_policy_moves(std::vector<Edge> &edges, size_t max_edges) noexcept
	{
		if (edges.size() <= max_edges)
			return;

		std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), [](const Edge &lhs, const Edge &rhs)
		{	return lhs.getPolicyPrior() > rhs.getPolicyPrior();}); // sort by prior policy in descending order
		edges.erase(edges.begin() + max_edges, edges.end());
	}

	void create_moves_from_solver(SearchTask &task) noexcept
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
	void create_moves_from_policy(SearchTask &task, float policyThreshold) noexcept
	{
		for (int row = 0; row < task.getBoard().rows(); row++)
			for (int col = 0; col < task.getBoard().cols(); col++)
				if (task.getBoard().at(row, col) == Sign::NONE and task.getPolicy().at(row, col) >= policyThreshold)
					task.addEdge(Move(row, col, task.getSignToMove()), task.getPolicy().at(row, col));
	}
	void check_terminal_conditions(SearchTask &task, Edge &edge)
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
	void override_proven_move_policy_and_value(const SearchTask &task, Edge &edge) noexcept
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
	bool is_in_center_square(const SearchTask &task, int size, int row, int col)
	{
		const int top = task.getBoard().rows() / 2 - size / 2;
		const int left = task.getBoard().cols() / 2 - size / 2;
		return row >= top and row < top + size and col >= left and col < left + size;
	}

	std::vector<int> get_symmetries(const matrix<Sign> &board)
	{
		std::vector<int> result;
		matrix<Sign> copy(board.rows(), board.cols());

		for (int i = 1; i < available_symmetries(board); i++)
		{
			augment(copy, board, i);
			if (copy == board)
				result.push_back(i);
		}
		return result;
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
		if (task.mustDefend() or task.isReadySolver())
		{ // we have some prior moves generated by the solver, use them
			if (task.getPriorEdges().size() == 0)
				throw std::runtime_error("there must be prior edges");
			create_moves_from_solver(task);
		}
		else
		{ // no moves were generated in the solver, use policy to create them
			assert(task.isReadyNetwork());
			create_moves_from_policy(task, policy_threshold);
		}
		// loop over all added moves to update policy and value for those that are proven
		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			override_proven_move_policy_and_value(task, *edge);
		if (not task.mustDefend())
			prune_low_policy_moves(task.getEdges(), max_edges);
		renormalize_policy(task.getEdges());

		if (task.getEdges().size() == 0) // TODO remove this later
		{
			std::cout << "---no-moves-generated---\n";
			std::cout << task.toString() << '\n';
			exit(-1);
		}

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
		assert(task.isReadyNetwork() or task.isReadySolver());

		if (task.getRelativeDepth() == 0)
		{
			if (task.mustDefend() or task.isReadySolver())
			{ // we have some prior moves generated by the solver, use them
				if (task.getPriorEdges().size() == 0)
					throw std::runtime_error("there must be prior edges");
				create_moves_from_solver(task);
			}
			else
			{ // no moves were generated in the solver, use policy to create them
				assert(task.isReadyNetwork());
				create_moves_from_policy(task, 0.0f);
			}
//			create_moves_from_policy(task, 0.0f);

			for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
			{
				check_terminal_conditions(task, *edge);
				override_proven_move_policy_and_value(task, *edge);
				const Move m = edge->getMove();
				edge->setPolicyPrior((1.0f - noise_weight) * edge->getPolicyPrior() + noise_weight * noise_matrix.at(m.row, m.col));
			}
			renormalize_policy(task.getEdges());
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

	CenterExcludingGenerator::CenterExcludingGenerator(int squareSize, const EdgeGenerator &baseGenerator) :
			square_size(squareSize),
			base_generator(std::unique_ptr<EdgeGenerator>(baseGenerator.clone()))
	{
	}
	CenterExcludingGenerator* CenterExcludingGenerator::clone() const
	{
		return new CenterExcludingGenerator(square_size, *base_generator);
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

	CenterOnlyGenerator::CenterOnlyGenerator(int squareSize, const EdgeGenerator &baseGenerator) :
			square_size(squareSize),
			base_generator(std::unique_ptr<EdgeGenerator>(baseGenerator.clone()))
	{
	}
	CenterOnlyGenerator* CenterOnlyGenerator::clone() const
	{
		return new CenterOnlyGenerator(square_size, *base_generator);
	}
	void CenterOnlyGenerator::generate(SearchTask &task) const
	{
		assert(task.isReadyNetwork() or task.isReadySolver());

		if (task.getRelativeDepth() == 0)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE and is_in_center_square(task, square_size, row, col))
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

	SymmetricalExcludingGenerator::SymmetricalExcludingGenerator(const EdgeGenerator &baseGenerator) :
			base_generator(std::unique_ptr<EdgeGenerator>(baseGenerator.clone()))
	{
	}
	SymmetricalExcludingGenerator* SymmetricalExcludingGenerator::clone() const
	{
		return new SymmetricalExcludingGenerator(*base_generator);
	}
	void SymmetricalExcludingGenerator::generate(SearchTask &task) const
	{
		assert(task.isReadyNetwork() or task.isReadySolver());

		if (task.getRelativeDepth() == 0)
		{
			std::vector<Move> tmp_moves; // a list of candidates, originally filled with all legal moves
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						tmp_moves.emplace_back(row, col, task.getSignToMove());

			const std::vector<int> symmetry_modes = get_symmetries(task.getBoard());
			for (size_t i = 0; i < symmetry_modes.size(); i++)
				for (size_t j = 0; j < tmp_moves.size(); j++)
				{
					const Move augmented = augment(tmp_moves[j], task.getBoard().rows(), task.getBoard().cols(), symmetry_modes[i]);
					auto position = std::find(tmp_moves.begin() + j + 1, tmp_moves.end(), augmented);
					if (position != tmp_moves.end())
						tmp_moves.erase(position);
				}
			for (size_t i = 0; i < tmp_moves.size(); i++)
				task.addEdge(tmp_moves[i]);

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

