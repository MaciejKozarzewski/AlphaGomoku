/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Edge.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

namespace ag
{
	std::string Edge::toString() const
	{
		std::string result = getMove().text();
		if (getMove().row < 10)
			result += "  : ";
		else
			result += " : ";
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
		result += " : Visits=" + std::to_string(getVisits());
		result += " : P=" + std::to_string(getPolicyPrior());
		return result;
	}
} /* namespace ag */

