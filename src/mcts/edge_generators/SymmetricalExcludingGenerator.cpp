/*
 * SymmetricalExcludingGenerator.cpp
 *
 *  Created on: Jan 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/edge_generators/SymmetricalExcludingGenerator.hpp>
#include <alphagomoku/mcts/edge_generators/generator_utils.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cassert>
#include <iostream>

namespace
{
	using namespace ag;

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
