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
	struct MaxPolicyPrior
	{
			float operator()(const Edge &edge) const noexcept
			{
				return edge.getPolicyPrior();
			}
	};

	void prune_weak_moves(std::vector<Edge> &edges, size_t max_edges) noexcept
	{
		if (edges.size() <= max_edges)
			return;

		MaxPolicyPrior op;
		std::partial_sort(edges.begin(), edges.begin() + max_edges, edges.end(), EdgeComparator<MaxPolicyPrior>(op));
		edges.erase(edges.begin() + max_edges, edges.end());
	}

	void create_legal_edges(SearchTask &task, bool prune) noexcept
	{
		if (task.getScore().isWin() and prune)
		{ // optimization to not add unnecessary edges if there is a win
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getActionScores().at(row, col).isWin())
					{
						assert(task.getBoard().at(row, col) == Sign::NONE);
						task.addEdge(Move(row, col, task.getSignToMove()));
					}
		}
		else
		{
			for (int row = 0; row < task.getBoard().rows(); row++)
				for (int col = 0; col < task.getBoard().cols(); col++)
					if (task.getBoard().at(row, col) == Sign::NONE)// and task.getPolicy().at(row, col) >= 1.0e-4f)
						task.addEdge(Move(row, col, task.getSignToMove()));
		}
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
		size_t num_losing_edges = 0;
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
		if (num_losing_edges == task.getEdges().size())
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
	BaseGenerator::BaseGenerator(int maxEdges, bool fullyExpandRoot) :
			max_edges(maxEdges),
			fully_expand_root(fullyExpandRoot)
	{
	}
	std::unique_ptr<EdgeGenerator> BaseGenerator::clone() const
	{
		return std::make_unique<BaseGenerator>(max_edges, fully_expand_root);
	}
	void BaseGenerator::generate(SearchTask &task) const
	{
		assert(task.isReady());

		if (task.mustDefend() and task.getRelativeDepth() > 0)
		{
			assert(task.wasProcessedBySolver());
			for (size_t i = 0; i < task.getDefensiveMoves().size(); i++)
				task.addEdge(task.getDefensiveMoves()[i]);
			initialize_edges(task);
		}
		else
		{
			const int num = (task.getRelativeDepth() == 0 and fully_expand_root) ? std::numeric_limits<int>::max() : max_edges;
			const bool early_prune = (task.getRelativeDepth() != 0);

			create_legal_edges(task, early_prune);
			initialize_edges(task);
			if (not task.wasProcessedBySolver())
				check_terminal_conditions(task);
			prune_weak_moves(task.getEdges(), num);
		}
		renormalize_policy(task.getEdges());

		if (task.getEdges().size() == 0) // TODO remove this later
		{
			std::cout << "---no-moves-generated---\n";
			std::cout << task.toString() << '\n';
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
			BaseGenerator(std::numeric_limits<int>::max(), true).generate(task);
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
