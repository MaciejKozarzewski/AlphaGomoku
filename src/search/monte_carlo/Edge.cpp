/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;

	std::string format_score(Score s)
	{
		switch (s.getProvenValue())
		{
			case ProvenValue::LOSS:
				return "LOSS in " + sfill(s.getEval(), 3, false);
			case ProvenValue::DRAW:
				return "DRAW       ";
			default:
			case ProvenValue::UNKNOWN:
				return sfill(s.getEval(), 4, true) + "       ";
			case ProvenValue::WIN:
				return "WIN  in " + sfill(s.getEval(), 3, false);
		}
	}
}

namespace ag
{
	std::string Edge::toString() const
	{
		std::string result = getMove().text();
		if (getMove().row < 10)
			result += " ";
		result += " : S=" + getScore().toString();
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
		result.node = nullptr;
		return result;
	}
} /* namespace ag */

