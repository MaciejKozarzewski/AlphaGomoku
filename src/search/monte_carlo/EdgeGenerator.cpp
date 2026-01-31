/*
 * EdgeGenerator.cpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <algorithm>
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
	struct MaxPolicyPrior
	{
			float operator()(const Edge &edge) const noexcept
			{
				return edge.getPolicyPrior();
			}
	};

	void prune_weak_moves(SearchTask &task, size_t max_edges, float expansionThreshold) noexcept
	{
		std::vector<Edge> &edges = task.getEdges();

		size_t idx = 0;
		if (task.getScore().isProven())
		{
			// at first find best score
			Score best_score = Score::loss();
			for (auto iter = edges.begin(); iter < edges.end(); iter++)
				best_score = std::max(best_score, iter->getScore());

			// then move all best edges to the front of the list
			for (size_t i = 0; i < edges.size(); i++)
				if (edges[i].getScore() == best_score)
				{
					std::swap(edges[i], edges[idx]);
					idx++;
				}
		}
		else
		{
			if (edges.size() <= max_edges or task.mustDefend())
				return;

			const MaxPolicyPrior op;
			std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), EdgeComparator<MaxPolicyPrior>(op));

			float sum_policy = 0.0f;
			for (size_t i = 0; i < max_edges; i++)
				sum_policy += edges[i].getPolicyPrior();
			expansionThreshold *= sum_policy; // scale the expansion threshold accordingly to the sum of remaining policy to avoid pruning all moves if the policy is low everywhere
			for (size_t i = 0; i < max_edges; i++)
				if (edges[i].getPolicyPrior() >= expansionThreshold)
					idx++;
		}
		edges.erase(edges.begin() + idx, edges.end());
	}

	void initialize_edges(SearchTask &task) noexcept
	{
		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
		{
			const Move m = edge->getMove();
			edge->setPolicyPrior(task.getPolicy().at(m.row, m.col));
			edge->setValue(task.getActionValues().at(m.row, m.col));
			edge->setScore(task.getActionScores().at(m.row, m.col));
		}
	}
	void check_terminal_conditions(SearchTask &task)
	{
		bool has_win_edge = false;
		bool has_draw_edge = false;
		int num_losing_edges = 0;
		for (auto edge = task.getEdges().begin(); edge < task.getEdges().end(); edge++)
		{
			const Move move = edge->getMove();

			Board::putMove(task.getBoard(), move);
			const GameOutcome outcome = getOutcome(task.getGameConfig().rules, task.getBoard(), move, task.getGameConfig().draw_after);
			Board::undoMove(task.getBoard(), move);

			switch (convertProvenValue(outcome, task.getSignToMove()))
			{
				case ProvenValue::UNKNOWN:
					break;
				case ProvenValue::LOSS:
					num_losing_edges++;
					edge->setScore(Score::loss_in(1));
					edge->setValue(Value::loss());
					break;
				case ProvenValue::DRAW:
					has_draw_edge = true;
					edge->setScore(Score::draw_in(1));
					edge->setValue(Value::draw());
					break;
				case ProvenValue::WIN:
					has_win_edge = true;
					edge->setScore(Score::win_in(1));
					edge->setValue(Value::win());
					break;
			}
		}
		if (has_win_edge)
		{
			task.setScore(Score::win_in(1));
			task.setValue(Value::win());
			return;
		}
		if (has_draw_edge)
		{
			task.setScore(Score::draw_in(1));
			task.setValue(Value::draw());
			return;
		}
		if (num_losing_edges == Board::numberOfMoves(task.getBoard()))
		{
			task.setScore(Score::loss_in(1));
			task.setValue(Value::loss());
		}
	}

	bool is_in_center_square(const SearchTask &task, int size, int row, int col)
	{
		const int top = task.getBoard().rows() / 2 - size / 2;
		const int left = task.getBoard().cols() / 2 - size / 2;
		return row >= top and row < top + size and col >= left and col < left + size;
	}
	std::vector<Symmetry> get_symmetries(const matrix<Sign> &board)
	{
		std::vector<Symmetry> result;
		matrix<Sign> copy(board.rows(), board.cols());

		for (int i = 1; i < number_of_available_symmetries(board.shape()); i++)
		{
			const Symmetry s = int_to_symmetry(i);
			apply_symmetry(copy, board, s);
			if (copy == board)
				result.push_back(s);
		}
		return result;
	}
}

namespace ag
{
	BaseGenerator::BaseGenerator(int maxEdges, float expansionThreshold, bool forceExpandRoot) :
			max_edges(maxEdges),
			expansion_threshold(expansionThreshold),
			force_expand_root(forceExpandRoot)
	{
	}
	std::unique_ptr<EdgeGenerator> BaseGenerator::clone() const
	{
		return std::make_unique<BaseGenerator>(max_edges, expansion_threshold, force_expand_root);
	}
	void BaseGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.mustDefend())
		{
			assert(task.wasProcessedBySolver());
			for (size_t i = 0; i < task.getDefensiveMoves().size(); i++)
				task.addEdge(task.getDefensiveMoves()[i]);
		}
		else
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						task.addEdge(Move(row, col, task.getSignToMove()));
		}
		initialize_edges(task);
		if (not task.wasProcessedBySolver())
			check_terminal_conditions(task);
		const bool expand_fully = (task.getRelativeDepth() == 0 and force_expand_root);
		if (not expand_fully) // do not prune root if such option is turned on
			prune_weak_moves(task, max_edges, expansion_threshold);
		renormalize_policy(task.getEdges());

		if (task.getEdges().size() == 0) // TODO remove this later
		{
			std::cout << "---no-moves-generated---\n";
			std::cout << task.toString() << '\n';
			for (int i = 0; i < 15; i++)
			{
				for (int j = 0; j < 15; j++)
					std::cout << task.getPolicy().at(i, j) << ' ';
				std::cout << '\n';
			}
			std::cout << Board::toString(task.getBoard(), true) << std::endl;
			exit(-1);
		}

		assert(task.getEdges().size() > 0);
	}

	UnifiedGenerator::UnifiedGenerator(int maxEdges, float expansionThreshold, bool forceExpandRoot) :
			max_edges(maxEdges),
			expansion_threshold(expansionThreshold),
			force_expand_root(forceExpandRoot)
	{
	}
	std::unique_ptr<EdgeGenerator> UnifiedGenerator::clone() const
	{
		return std::make_unique<UnifiedGenerator>(max_edges, expansion_threshold, force_expand_root);
	}
	void UnifiedGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (not task.wasProcessedBySolver())
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						task.addEdge(Move(row, col, task.getSignToMove()));
		}
		initialize_edges(task);
		if (not task.wasProcessedBySolver())
			check_terminal_conditions(task);
		const bool expand_fully = (task.getRelativeDepth() == 0 and force_expand_root);
		if (not expand_fully) // do not prune root if such option is turned on
			prune_weak_moves(task, max_edges, expansion_threshold);
		renormalize_policy(task.getEdges());

		if (task.getEdges().size() == 0) // TODO remove this later
		{
			std::cout << "---no-moves-generated---\n";
			std::cout << task.toString() << '\n';
			for (int i = 0; i < 15; i++)
			{
				for (int j = 0; j < 15; j++)
					std::cout << task.getPolicy().at(i, j) << ' ';
				std::cout << '\n';
			}
			std::cout << Board::toString(matrix<Sign>(15, 15), task.getPolicy());
			exit(-1);
		}

		assert(task.getEdges().size() > 0);
	}

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
		assert(task.isReady());
		if (task.getAbsoluteDepth() < balance_depth)
			BaseGenerator(std::numeric_limits<int>::max(), 0.0f).generate(task);
		else
			base_generator->generate(task);
	}

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
		assert(task.isReady());

		if (task.getRelativeDepth() == 0)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE and not is_in_center_square(task, square_size, row, col))
						task.addEdge(Move(row, col, task.getSignToMove()));

			initialize_edges(task);
			if (not task.wasProcessedBySolver())
				check_terminal_conditions(task);
			renormalize_policy(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);
	}

	CenterOnlyGenerator::CenterOnlyGenerator(int squareSize, const EdgeGenerator &baseGenerator) :
			square_size(squareSize),
			base_generator(baseGenerator.clone())
	{
	}
	std::unique_ptr<EdgeGenerator> CenterOnlyGenerator::clone() const
	{
		return std::make_unique<CenterOnlyGenerator>(square_size, *base_generator);
	}
	void CenterOnlyGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getRelativeDepth() == 0)
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE and is_in_center_square(task, square_size, row, col))
						task.addEdge(Move(row, col, task.getSignToMove()));

			initialize_edges(task);
			if (not task.wasProcessedBySolver())
				check_terminal_conditions(task);
			renormalize_policy(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);
	}

	SymmetricalExcludingGenerator::SymmetricalExcludingGenerator(const EdgeGenerator &baseGenerator) :
			base_generator(baseGenerator.clone())
	{
	}
	std::unique_ptr<EdgeGenerator> SymmetricalExcludingGenerator::clone() const
	{
		return std::make_unique<SymmetricalExcludingGenerator>(*base_generator);
	}
	void SymmetricalExcludingGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.getRelativeDepth() == 0)
		{
			std::vector<Move> tmp_moves; // a list of candidates, originally filled with all legal moves
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)
						tmp_moves.emplace_back(row, col, task.getSignToMove());

			const std::vector<Symmetry> symmetry_modes = get_symmetries(task.getBoard());
			for (size_t i = 0; i < symmetry_modes.size(); i++)
				for (size_t j = 0; j < tmp_moves.size(); j++)
				{
					const Move augmented = apply_symmetry(tmp_moves[j], task.getBoard().shape(), symmetry_modes[i]);
					auto position = std::find(tmp_moves.begin() + j + 1, tmp_moves.end(), augmented);
					if (position != tmp_moves.end())
						tmp_moves.erase(position);
				}
			for (size_t i = 0; i < tmp_moves.size(); i++)
				task.addEdge(tmp_moves[i]);

			initialize_edges(task);
			if (not task.wasProcessedBySolver())
				check_terminal_conditions(task);
			renormalize_policy(task.getEdges());
			assert(task.getEdges().size() > 0);
		}
		else
			base_generator->generate(task);
	}

} /* namespace ag */
