/*
 * SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>

namespace ag
{
	void SearchTask::reset(const Board &base) noexcept
	{
		visited_pairs.clear();
		board = base;
		last_move = Move();
		policy.clear();
		value = Value();
		proven_value = ProvenValue::UNKNOWN;
		is_ready = false;
	}
	void SearchTask::correctInformationLeak() noexcept
	{
		assert(size() > 0);
		Edge *edge = getLastPair().edge;
		Node *node = edge->getNode();
		assert(node !=nullptr);
		Value edgeQ = edge->getValue();
		Value nodeQ = node->getValue();

		this->value = (nodeQ - edgeQ) * edge->getVisits() + nodeQ;
		this->proven_value = node->getProvenValue();
		is_ready = true;
	}
	void SearchTask::chackIfTerminal() noexcept
	{
		GameOutcome outcome;
		if (size() > 0)
			outcome = board.getOutcome(last_move);
		else
			outcome = board.getOutcome();
		if (outcome != GameOutcome::UNKNOWN)
		{
			setReady();
			setValue(convertOutcome(outcome, getSignToMove()));
			setProvenValue(convertProvenValue(outcome, getSignToMove()));
		}
	}
	std::string SearchTask::toString() const
	{
		std::string result;
		for (int i = 0; i < size(); i++)
		{
			result += getPair(i).node->toString() + '\n';
			result += getPair(i).edge->toString() + '\n';
		}
		return result;
	}
} /* namespace ag */

