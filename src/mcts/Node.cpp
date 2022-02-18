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

namespace
{
	double expectation(const ag::Edge &edge) noexcept
	{
		return edge.getWinRate() + 0.5f * edge.getDrawRate();
	}
	double is_win(const ag::Edge &edge) noexcept
	{
		return static_cast<double>(edge.getProvenValue() == ag::ProvenValue::WIN);
	}
	double is_loss(const ag::Edge &edge) noexcept
	{
		return static_cast<double>(edge.getProvenValue() == ag::ProvenValue::LOSS);
	}
	double get_edge_value(const ag::Edge &edge) noexcept
	{
		return edge.getVisits() + expectation(edge) + 0.001 * edge.getPolicyPrior() + (is_win(edge) - is_loss(edge)) * 1e9;
	}
}

namespace ag
{
	std::string Node::toString() const
	{
		std::string result = "depth=";
		result += std::to_string(getDepth()) + " : ";
		switch (getProvenValue())
		{
			default:
			case ProvenValue::UNKNOWN:
				result += 'u';
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
		result += " : Edges=" + std::to_string(numberOfEdges());
#ifndef NDEBUG
		result += " : now moving=" + ag::toString(sign_to_move);
#endif
		return result;
	}
	void Node::sortEdges() const
	{
		auto expectation = [](const Edge &e)
		{
			return e.getWinRate() + 0.5f * e.getDrawRate();
		};
		std::sort(this->begin(), this->end(), [expectation](const Edge &lhs, const Edge &rhs)
		{	return get_edge_value(lhs) > get_edge_value(rhs);});
	}

} /* namespace ag */

