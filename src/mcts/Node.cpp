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
	std::string Node::toString() const
	{
		std::string result = Move(getMove()).toString() + " : ";
		switch (getProvenValue())
		{
			case ProvenValue::UNKNOWN:
				result += 'U';
				break;
			case ProvenValue::WIN:
				result += 'W';
				break;
			case ProvenValue::LOSS:
				result += 'L';
				break;
			case ProvenValue::DRAW:
				result += 'D';
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
//		if (number == -1)
//			number = numberOfChildren();
		std::sort(children, children + numberOfChildren(), [](const Node &lhs, const Node &rhs)
		{	return (lhs.visits + lhs.value + 0.1f * lhs.policy_prior) > (rhs.visits + rhs.value + 0.1f * rhs.policy_prior);});

//		number = std::min(number, numberOfChildren());
//		std::cout << "Children of node : " << toString() << '\n';
//		if (2 * number >= numberOfChildren())
//		{
//			for (int i = 0; i < numberOfChildren(); i++)
//				std::cout << "--" << children[i].toString() << '\n';
//		}
//		else
//		{
//			std::cout << "BEST" << std::endl;
//			for (int i = 0; i < number; i++)
//				std::cout << "--" << children[i].toString() << '\n';
//			std::cout << "WORST" << std::endl;
//			for (int i = std::max(0, numberOfChildren() - number); i < numberOfChildren(); i++)
//				std::cout << "--" << children[i].toString() << '\n';
//		}
	}
}

