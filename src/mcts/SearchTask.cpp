/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>

namespace ag
{
	void SearchTask::reset(const matrix<Sign> &base, Sign signToMove) noexcept
	{
		visited_pairs.clear();
		board = base;
		sign_to_move = signToMove;
		if (policy.size() == board.size())
			policy.clear();
		else
			policy = matrix<float>(base.rows(), base.cols());
		value = Value();
		proven_value = ProvenValue::UNKNOWN;
		is_ready = false;
	}
	std::string SearchTask::toString() const
	{
		std::string result;
		for (int i = 0; i < size(); i++)
		{
			result += getPair(i).node->toString() + '\n';
			result += "---" + getPair(i).edge->toString() + '\n';
		}
		result += "sign to move = " + getSignToMove() + '\n';
		if (is_ready)
		{
			result += "value = " + value.toString();
			if (proven_value != ProvenValue::UNKNOWN)
				result += " : proven " + ag::toString(proven_value);
			result += '\n';
			result += Board::toString(board, policy);
		}
		else
			result += Board::toString(board);
		return result;
	}
} /* namespace ag */

