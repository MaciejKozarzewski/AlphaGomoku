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
	struct edge_value
	{
			double operator()(const Edge &edge) const noexcept
			{
				return edge.getVisits() + edge.getExpectation() + 0.001 * edge.getPolicyPrior();
			}
	};
	[[maybe_unused]] std::string flags_to_string(uint16_t flags)
	{
		std::string result;
		result += std::to_string((flags & 1u) != 0);
		result += std::to_string((flags & 2u) != 0);
		result += std::to_string((flags & 4u) != 0);
		return result;
	}
}

namespace ag
{
	std::string Node::toString() const
	{
		std::string result = "depth=";
		result += std::to_string(getDepth());
		result += " : S=" + getScore().toFormattedString();
		result += " : Q=" + getValue().toString();
		result += " : Visits=" + std::to_string(getVisits());
#ifndef NDEBUG
		result += " (" + std::to_string(getVirtualLoss()) + ")";
#endif
		result += " : Edges=" + std::to_string(numberOfEdges());
#ifndef NDEBUG
		result += " : flags=" + flags_to_string(flags);
		result += " : now moving=" + ag::toString(sign_to_move);
#endif
		return result;
	}
	void Node::sortEdges() const
	{
		edge_value op;
		EdgeComparator<edge_value> comp(op);
		std::sort(this->begin(), this->end(), comp);
	}

	void copyEdgeInfo(Node &dst, const Node &src)
	{
		dst.createEdges(src.numberOfEdges());
		for (int i = 0; i < src.numberOfEdges(); i++)
			dst.getEdge(i) = src.getEdge(i).copyInfo();
	}

} /* namespace ag */

