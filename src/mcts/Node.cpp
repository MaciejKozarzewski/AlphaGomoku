/*
 * Node.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace ag
{
	std::string toString(ProvenValue pv)
	{
		switch (pv)
		{
			default:
			case ProvenValue::UNKNOWN:
				return "UNKNOWN";
			case ProvenValue::LOSS:
				return "LOSS";
			case ProvenValue::DRAW:
				return "DRAW";
			case ProvenValue::WIN:
				return "WIN";
		}
	}

	std::string Node::toString() const
	{
		std::string result = Move(getMove()).toString() + " : ";
		switch (getProvenValue())
		{
			default:
			case ProvenValue::UNKNOWN:
				result += 'U';
				break;
			case ProvenValue::LOSS:
				result += 'L';
				break;
			case ProvenValue::DRAW:
				result += 'D';
				break;
			case ProvenValue::WIN:
				result += 'W';
				break;
		}
		result += " : Q=" + std::to_string(value);
		result += " : H=" + std::to_string(policy_prior);
		result += " : Visits=" + std::to_string(visits);
		result += " : Children=" + std::to_string(numberOfChildren());
		return result;
	}
	void Node::sortChildren() const
	{
		std::sort(children, children + numberOfChildren(), [](const Node &lhs, const Node &rhs)
		{	return (lhs.visits + lhs.value + 0.001f * lhs.policy_prior) > (rhs.visits + rhs.value + 0.001f * rhs.policy_prior);});
	}
}

