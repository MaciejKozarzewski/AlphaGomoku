/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/SearchTask.hpp>

#include <cstring>
#include <cassert>

namespace
{
	using namespace ag;
	template<typename T>
	void clear_or_reallocate(T &m, const matrix<Sign> &board)
	{
		if (equalSize(m, board))
			m.clear();
		else
			m = T(board.rows(), board.cols());
	}
}

namespace ag
{
	SearchTask::SearchTask(GameConfig config) :
			game_config(config)
	{
	}
	void SearchTask::set(const matrix<Sign> &base, Sign signToMove)
	{
		visited_path.clear();
		edges.clear();
		board = base;
		clear_or_reallocate(features, board);
		clear_or_reallocate(policy, board);
		clear_or_reallocate(action_values, board);
		value = Value();
		clear_or_reallocate(action_scores, board);
		score = Score();
		sign_to_move = signToMove;
		was_processed_by_network = false;
		was_processed_by_solver = false;
		must_defend = false;
	}

	void SearchTask::append(Node *node, Edge *edge)
	{
		assert(node != nullptr);
		assert(edge != nullptr);
		const Move m = edge->getMove();
		assert(m.sign == sign_to_move);
		Board::putMove(board, m);
		sign_to_move = invertSign(m.sign);
		visited_path.push_back(NodeEdgePair( { node, edge }));
	}
	void SearchTask::addEdge(Move move)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(edges.begin(), edges.end(), [move](const Edge &edge) { return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		edges.push_back(e);
	}
	std::string SearchTask::toString() const
	{
		std::string result;
		for (int i = 0; i < visitedPathLength(); i++)
		{
			result += getPair(i).node->toString() + '\n';
			result += "---" + getPair(i).edge->toString() + '\n';
		}
		result += "Sign to move = " + getSignToMove() + '\n';
		if (wasProcessedByNetwork() or wasProcessedBySolver())
		{
			if (wasProcessedByNetwork())
				result += "Evaluated by neural network\n";
			if (wasProcessedBySolver())
				result += "Evaluated by alpha-beta search\n";
			if (must_defend)
				result += "Must defend\n";
			result += "Value = " + value.toString() + '\n';
			result += "Score = " + score.toString() + '\n';
			result += "Policy:\n" + Board::toString(board, policy);
			result += "Action values:\n" + Board::toString(board, action_values);
			result += "Action scores:\n" + Board::toString(board, action_scores);
		}
		else
			result += Board::toString(board, true);
		if (edges.size() > 0)
		{
			result += "Generated moves:\n";
			for (size_t i = 0; i < edges.size(); i++)
				result += edges[i].toString() + '\n';
		}
		return result;
	}

} /* namespace ag */

