/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Edge.hpp>

namespace ag
{
	std::string Edge::toString() const
	{
		std::string result = "Edge : " + Move(getMove()).toString() + " : ";
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
		result += " : Q=" + getValue().toString();
		result += " : P=" + std::to_string(getPolicyPrior());
		result += " : Visits=" + std::to_string(getVisits());
		return result;
	}
} /* namespace ag */

