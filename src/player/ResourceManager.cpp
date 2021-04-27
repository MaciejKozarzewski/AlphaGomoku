/*
 * TimeManager.cpp
 *
 *  Created on: 17 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/player/ResourceManager.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <algorithm>

namespace ag
{

	void ResourceManager::setSearchStartTime(double t) noexcept
	{
		search_start_time = t;
	}
	double ResourceManager::getElapsedTime() const noexcept
	{
		return getTime() - search_start_time;
	}
	uint64_t ResourceManager::getMaxMemory() const noexcept
	{
		return max_memory;
	}
	double ResourceManager::getTimeForTurn() const noexcept
	{
		if (time_for_turn == 0) // timeout turn set to 0 - play as fast as possible
			return MIN_TIME_FOR_MOVE;
		else
		{
			if (time_for_match == 0) // there is no match time limit
				return (double) time_for_turn - PROTOCOL_OVERHEAD; // just use timeout turn
			else
				return std::min(time_for_turn, (TIME_FRACTION * time_left)) - PROTOCOL_OVERHEAD;
		}
	}
	double ResourceManager::getTimeForSwap2(int stones) const noexcept
	{
		switch (stones)
		{
			case 0:
				return 0.0;
			case 3:
				return SWAP2_FRACTION * time_left;
			case 5:
				return TIME_FRACTION * time_left;
			default:
				return 0.0;
		}
	}

	void ResourceManager::setMaxMemory(uint64_t m) noexcept
	{
		max_memory = m;
	}
	void ResourceManager::setTimeForMatch(double t) noexcept
	{
		time_for_match = t;
	}
	void ResourceManager::setTimeForTurn(double t) noexcept
	{
		time_for_turn = t;
	}
	void ResourceManager::setTimeLeft(double t) noexcept
	{
		time_left = t;
	}

} /* namespace ag */

