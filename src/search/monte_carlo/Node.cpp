/*
 * Node.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <algorithm>
#include <cmath>

namespace
{
	using namespace ag;
	double get_edge_value(const Edge &edge) noexcept
	{
		return edge.getVisits() + edge.getValue().getExpectation() + 0.001 * edge.getPolicyPrior();
	}

	struct CompareEdges
	{
			bool operator()(const Edge &lhs, const Edge &rhs) const noexcept
			{
				if (lhs.getScore() == rhs.getScore())
					return get_edge_value(lhs) > get_edge_value(rhs);
				else
					return lhs.getScore() > rhs.getScore();
			}
	};

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
				return sfill(s.getEval(), 4, true);
			case ProvenValue::WIN:
				return "WIN  in " + sfill(s.getEval(), 3, false);
		}
	}
}

namespace ag
{
	std::string Node::toString() const
	{
		std::string result = "depth=";
		result += std::to_string(getDepth());
		result += " : S=" + getScore().toString();
		result += " : Q=" + getValue().toString();
		result += " : Visits=" + std::to_string(getVisits());
		result += " : Edges=" + std::to_string(numberOfEdges());
#ifndef NDEBUG
		result += " : now moving=" + ag::toString(sign_to_move);
#endif
		return result;
	}
	void Node::sortEdges() const
	{
		std::sort(this->begin(), this->end(), CompareEdges());
	}

} /* namespace ag */

