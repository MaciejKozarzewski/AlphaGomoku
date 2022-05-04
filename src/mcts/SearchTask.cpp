/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>

#include <cassert>

namespace ag
{
	SearchTask::SearchTask(GameRules rules) :
			game_rules(rules)
	{
	}
	void SearchTask::set(const matrix<Sign> &base, Sign signToMove)
	{
		visited_path.clear();
		proven_edges.clear();
		edges.clear();
		board = base;
		sign_to_move = signToMove;
		if (equalSize(policy, board))
			policy.clear();
		else
			policy = matrix<float>(base.rows(), base.cols());
		if (equalSize(action_values, board))
			action_values.clear();
		else
			action_values = matrix<Value>(base.rows(), base.cols());
		value = Value();
		is_ready = false;
	}

	void SearchTask::append(Node *node, Edge *edge)
	{
		assert(node != nullptr);
		assert(edge != nullptr);
		Move m = edge->getMove();
		assert(m.sign == sign_to_move);
		Board::putMove(board, m);
		sign_to_move = invertSign(m.sign);
		visited_path.push_back(NodeEdgePair( { node, edge }));
	}
	void SearchTask::setPolicy(const float *p) noexcept
	{
		assert(p != nullptr);
		std::memcpy(policy.data(), p, policy.sizeInBytes());
	}
	void SearchTask::setActionValues(const float *q) noexcept
	{
		assert(q != nullptr);
		std::memcpy(reinterpret_cast<float*>(action_values.data()), q, action_values.sizeInBytes());
	}
	void SearchTask::addNonLosingEdges(const std::vector<Move> &moves, Sign sign, ProvenValue pv)
	{
		for (size_t i = 0; i < moves.size(); i++)
			this->addNonLosingEdge(Move(sign, moves[i]), pv);
	}
	void SearchTask::addNonLosingEdge(Move move, ProvenValue pv)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(proven_edges.begin(), proven_edges.end(), [move](const Edge &edge)
		{	return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		e.setProvenValue(pv);
		non_losing_edges.push_back(e);
	}
	void SearchTask::addProvenEdges(const std::vector<Move> &moves, Sign sign, ProvenValue pv)
	{
		for (size_t i = 0; i < moves.size(); i++)
			this->addProvenEdge(Move(sign, moves[i]), pv);
	}
	void SearchTask::addProvenEdge(Move move, ProvenValue pv)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(proven_edges.begin(), proven_edges.end(), [move](const Edge &edge)
		{	return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		e.setProvenValue(pv);
		proven_edges.push_back(e);
	}
	void SearchTask::addEdge(Move move, ProvenValue pv)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(edges.begin(), edges.end(), [move](const Edge &edge)
		{	return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		e.setProvenValue(pv);
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
		result += "sign to move = " + getSignToMove() + '\n';
		if (is_ready)
		{
			result += "value = " + value.toString();
			result += '\n';
			result += Board::toString(board, policy);
		}
		else
			result += Board::toString(board);
		if (proven_edges.size() > 0)
		{
			result += "Proven moves:\n";
			for (size_t i = 0; i < proven_edges.size(); i++)
				result += proven_edges[i].getMove().toString() + " : " + proven_edges[i].toString() + '\n';
		}
		if (edges.size() > 0)
		{
			result += "Generated moves:\n";
			for (size_t i = 0; i < edges.size(); i++)
				result += edges[i].toString() + '\n';
		}
		return result;
	}
} /* namespace ag */

