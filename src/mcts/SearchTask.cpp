/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>

namespace ag
{
	SearchTask::SearchTask(GameRules rules) :
			game_rules(rules)
	{
	}
	void SearchTask::reset(const matrix<Sign> &base, Sign signToMove) noexcept
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
	int SearchTask::visitedPathLength() const noexcept
	{
		return static_cast<int>(visited_path.size());
	}

	NodeEdgePair SearchTask::getPair(int index) const noexcept
	{
		assert(index >= 0 && index < visitedPathLength());
		return visited_path[index];
	}
	NodeEdgePair SearchTask::getLastPair() const noexcept
	{
		assert(visitedPathLength() > 0);
		return visited_path.back();
	}
	Edge* SearchTask::getLastEdge() const noexcept
	{
		if (visited_path.size() == 0)
			return nullptr;
		else
			return visited_path.back().edge;
	}

	const std::vector<Edge>& SearchTask::getProvenEdges() const noexcept
	{
		return proven_edges;
	}
	std::vector<Edge>& SearchTask::getProvenEdges() noexcept
	{
		return proven_edges;
	}

	const std::vector<Edge>& SearchTask::getEdges() const noexcept
	{
		return edges;
	}
	std::vector<Edge>& SearchTask::getEdges() noexcept
	{
		return edges;
	}

	GameRules SearchTask::getGameRules() const noexcept
	{
		return game_rules;
	}
	Sign SearchTask::getSignToMove() const noexcept
	{
		return sign_to_move;
	}
	Value SearchTask::getValue() const noexcept
	{
		return value;
	}
	bool SearchTask::isReady() const noexcept
	{
		return is_ready;
	}

	const matrix<Sign>& SearchTask::getBoard() const noexcept
	{
		return board;
	}
	matrix<Sign>& SearchTask::getBoard() noexcept
	{
		return board;
	}

	const matrix<Value>& SearchTask::getActionValues() const noexcept
	{
		return action_values;
	}
	matrix<Value>& SearchTask::getActionValues() noexcept
	{
		return action_values;
	}

	const matrix<float>& SearchTask::getPolicy() const noexcept
	{
		return policy;
	}
	matrix<float>& SearchTask::getPolicy() noexcept
	{
		return policy;
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
	void SearchTask::setReady() noexcept
	{
		is_ready = true;
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
	void SearchTask::setValue(Value v) noexcept
	{
		value = v;
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
	void SearchTask::addEdge(Move move)
	{
		assert(move.sign == sign_to_move);
		assert(std::none_of(edges.begin(), edges.end(), [move](const Edge &edge)
		{	return edge.getMove() == move;})); // an edge must not be added twice
		assert(board.at(move.row, move.col) == Sign::NONE); // move must be valid

		edges.push_back(Edge(move));
	}
	void SearchTask::addEdge(const Edge &other)
	{
		edges.push_back(other);
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
				result += proven_edges[i].toString() + '\n';
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

