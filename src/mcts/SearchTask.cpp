/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>

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
	SearchTask::SearchTask(GameRules rules) :
			game_rules(rules)
	{
	}
	void SearchTask::set(const matrix<Sign> &base, Sign signToMove)
	{
		visited_path.clear();
		prior_edges.clear();
		edges.clear();
		board = base;
		sign_to_move = signToMove;
		clear_or_reallocate(features, board);
		clear_or_reallocate(policy, board);
		clear_or_reallocate(action_values, board);
		value = Value();
		proven_value = ProvenValue::UNKNOWN;
		is_ready_solver = false;
		is_ready_network = false;
		must_defend = false;
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
	void SearchTask::addPriorEdge(Move move, Value v, ProvenValue pv)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(prior_edges.begin(), prior_edges.end(), [move](const Edge &edge) { return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		e.setValue(v);
		e.setProvenValue(pv);
		prior_edges.push_back(e);
	}
	void SearchTask::addEdge(Move move, float policyPrior, Value actionValue)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(edges.begin(), edges.end(), [move](const Edge &edge) { return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		Edge e;
		e.setMove(move);
		e.setPolicyPrior(policyPrior);
		e.setValue(actionValue);
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
		if (is_ready_network or is_ready_solver)
		{
			if (is_ready_network)
				result += "evaluated by neural network\n";
			if (is_ready_solver)
				result += "evaluated by alpha-beta search\n";
			if (must_defend)
				result += "must defend\n";
			result += "value = " + value.toString() + '\n';
			result += "proven value = " + ag::toString(proven_value) + '\n';
			result += "policy\n" + Board::toString(board, policy);
			result += "action values\n" + Board::toString(board, action_values);
		}
		else
			result += Board::toString(board, true);
		if (prior_edges.size() > 0)
		{
			result += "Prior moves:\n";
			for (size_t i = 0; i < prior_edges.size(); i++)
				result += prior_edges[i].getMove().toString() + " : " + prior_edges[i].toString() + '\n';
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

