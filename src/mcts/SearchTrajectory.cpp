/*
 * SearchTrajectory.cpp
 *
 *  Created on: Mar 3, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/SearchTrajectory.hpp>

namespace ag
{
	std::string SearchTrajectory::toString() const
	{
		std::string result;
		for (int i = 0; i < length(); i++)
		{
			result += "Depth ";
			if (i < 10 && length() > 10)
				result += ' ';
			if (i < 100 && length() > 100)
				result += ' ';
			result += std::to_string(i) + " : " + getNode(i).toString() + '\n';
		}
		return result;
	}
}

