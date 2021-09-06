/*
 * Node.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Node.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace ag
{
	std::string Node_old::toString() const
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
		result += " : Q=" + value.toString();
		result += " : P=" + std::to_string(policy_prior);
		result += " : Visits=" + std::to_string(visits);
		result += " : Children=" + std::to_string(numberOfChildren());
		return result;
	}
	void Node_old::sortChildren() const
	{
		MaxExpectation expectation;
		std::sort(children, children + numberOfChildren(), [expectation](const Node_old &lhs, const Node_old &rhs)
		{	double lhs_value = lhs.visits + expectation(&lhs) + 0.001 * lhs.policy_prior;
			lhs_value+= ((int)(lhs.getProvenValue() == ProvenValue::WIN) - (int)(lhs.getProvenValue() == ProvenValue::LOSS)) * 1e9;
			double rhs_value = rhs.visits + expectation(&rhs) + 0.001 * rhs.policy_prior;
			rhs_value+= ((int)(rhs.getProvenValue() == ProvenValue::WIN) - (int)(rhs.getProvenValue() == ProvenValue::LOSS)) * 1e9;
			return lhs_value > rhs_value;
		});
	}
}

