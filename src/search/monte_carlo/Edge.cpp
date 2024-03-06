/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/utils/misc.hpp>

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
#endif
		result += " : P=" + std::to_string(getPolicyPrior());
		return result;
	}
	Edge Edge::copyInfo() const noexcept
	{
		Edge result(*this);
//		result.node = nullptr;
		return result;
	}
} /* namespace ag */

