/*
 * data_packs.cpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>

#include <iostream>

namespace ag
{
	SearchDataPack::SearchDataPack(int rows, int cols) :
			board(rows, cols),
			policy_prior(rows, cols),
			visit_count(rows, cols),
			action_values(rows, cols),
			action_scores(rows, cols)
	{
	}
	SearchDataPack::SearchDataPack(const Node &rootNode, const matrix<Sign> &board) :
			SearchDataPack(board.rows(), board.cols())
	{
		this->board = board;
		for (const Edge *edge = rootNode.begin(); edge < rootNode.end(); edge++)
		{
			const Move m = edge->getMove();

			policy_prior.at(m.row, m.col) = edge->getPolicyPrior();
			visit_count.at(m.row, m.col) = edge->getVisits();
			action_values.at(m.row, m.col) = edge->getValue();
			action_scores.at(m.row, m.col) = edge->getScore();
		}
		minimax_value = rootNode.getValue();
		minimax_score = rootNode.getScore();
	}
	void SearchDataPack::clear()
	{
		board.clear();
		policy_prior.clear();
		visit_count.clear();
		action_values.clear();
		action_scores.clear();
		minimax_value = Value();
		minimax_score = Score();
		moves_left = 0;
		game_outcome = GameOutcome::UNKNOWN;
		played_move = Move();
	}
	void SearchDataPack::print() const
	{
		std::cout << "Played move: " << played_move.toString() << '\n';
		std::cout << "Game outcome: " << toString(game_outcome) << '\n';
		std::cout << "Minimax value: " << minimax_value.toString() << '\n';
		std::cout << "Minimax score: " << minimax_score.toString() << '\n';
		std::cout << "Moves left: " << moves_left << '\n';
		std::cout << "Board:\n" << Board::toString(board, true) << '\n';
		std::cout << "Policy prior:\n" << Board::toString(board, policy_prior, true) << '\n';
		std::cout << "Visit count:\n" << Board::toString(board, visit_count, true) << '\n';
		std::cout << "Action values:\n" << Board::toString(board, action_values, true) << '\n';
		std::cout << "Action scores:\n" << Board::toString(board, action_scores, true) << '\n';
	}
	int SearchDataPack::check_correctness() const noexcept
	{
		if (played_move.row < 0 or played_move.row >= board.rows())
			return 1;
		if (played_move.col < 0 or played_move.col >= board.cols())
			return 2;
		if (played_move.sign != Sign::CROSS and played_move.sign != Sign::CIRCLE)
			return 3;

		for (int i = 0; i < board.size(); i++)
		{
			if (policy_prior[i] < 0.0f or policy_prior[i] > 1.0f)
				return 1000 + i;
			if (visit_count[i] < 0)
				return 2000 + i;
			if (not action_values[i].isValid())
				return 3000 + i;
		}

		if (not minimax_value.isValid())
			return 11;

		return 0;
	}

	TrainingDataPack::TrainingDataPack(int rows, int cols) :
			board(rows, cols),
			visit_count(rows, cols),
			policy_target(rows, cols),
			action_values_target(rows, cols)
	{
	}
	void TrainingDataPack::clear()
	{
		board.clear();
		visit_count.clear();
		policy_target.clear();
		action_values_target.clear();
		value_target = Value();
		minimax_target = Value();
		moves_left = 0.0f;
		sign_to_move = Sign::NONE;
	}
	void TrainingDataPack::print() const
	{
		std::cout << "Sign to move: " << toString(sign_to_move) << '\n';
		std::cout << "Value target: " << value_target.toString() << '\n';
		std::cout << "Minimax target: " << minimax_target.toString() << '\n';
		std::cout << "Moves left: " << moves_left << '\n';
		std::cout << "Board:\n" << Board::toString(board, true) << '\n';
		std::cout << "Visit count:\n" << Board::toString(board, visit_count, true) << '\n';
		std::cout << "Policy target:\n" << Board::toString(board, policy_target, true) << '\n';
		std::cout << "Action values target:\n" << Board::toString(board, action_values_target, true) << '\n';
	}

} /* namespace ag */
