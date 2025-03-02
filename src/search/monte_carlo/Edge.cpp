/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/random.hpp>

#include <iostream>

namespace ag
{
	std::string Edge::toString() const
	{
		std::string result = getMove().text();
		if (getMove().row < 10)
			result += " ";
		result += " : S=" + getScore().toFormattedString();
		result += " : Q=" + getValue().toString();
		result += " : Visits=" + std::to_string(getVisits());
#ifndef NDEBUG
		result += " (" + std::to_string(getVirtualLoss()) + ")";
		result += " : flags=" + std::to_string(static_cast<int>(isBeingExpanded()));
#endif
		result += " : P=" + std::to_string(getPolicyPrior());
		return result;
	}

} /* namespace ag */

