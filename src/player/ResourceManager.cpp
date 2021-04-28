/*
 * TimeManager.cpp
 *
 *  Created on: 17 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/player/ResourceManager.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <libml/hardware/Device.hpp>

#include <algorithm>

namespace ag
{

	void ResourceManager::setRows(int rows) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.rows = rows;
	}
	void ResourceManager::setCols(int cols) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.cols = cols;
	}
	void ResourceManager::setRules(GameRules rules) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.rules = rules;
	}
	GameConfig ResourceManager::getGameConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return game_config;
	}

	void ResourceManager::setSearchStartTime(double t) noexcept
	{
		std::lock_guard lock(mutex);
		search_start_time = t;
	}
	double ResourceManager::getElapsedTime() const noexcept
	{
		std::lock_guard lock(mutex);
		return getTime() - search_start_time;
	}
	uint64_t ResourceManager::getMaxMemory() const noexcept
	{
		std::lock_guard lock(mutex);
		return max_memory;
	}
	double ResourceManager::getTimeForTurn() const noexcept
	{
		std::lock_guard lock(mutex);
		if (time_for_turn == 0) // timeout turn set to 0 - play as fast as possible
			return 0.0;
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
		std::lock_guard lock(mutex);
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
	double ResourceManager::getTimeForPondering() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_for_pondering;
	}

	void ResourceManager::setMaxMemory(uint64_t m) noexcept
	{
		std::lock_guard lock(mutex);
		max_memory = m;
	}
	void ResourceManager::setTimeForMatch(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_match = t;
	}
	void ResourceManager::setTimeForTurn(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_turn = t;
	}
	void ResourceManager::setTimeLeft(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_left = t;
	}
	void ResourceManager::setTimeForPondering(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_pondering = t;
	}

	bool ResourceManager::setOption(const Option &option) noexcept
	{
		try
		{
			if (option.name == "time_for_turn")
			{
				setTimeForTurn(std::stod(option.value) / 1000.0);
				return true;
			}
			if (option.name == "time_for_match")
			{
				if (std::stod(option.value) == 0)
					setTimeForMatch(2147483647.0);
				else
					setTimeForMatch(std::stod(option.value) / 1000.0);
				return true;
			}
			if (option.name == "time_left")
			{
				setTimeLeft(std::stod(option.value) / 1000.0);
				return true;
			}
			if (option.name == "time_for_pondering")
			{
				if (std::stod(option.value) == -1)
					setTimeForPondering(2147483647.0);
				else
					setTimeForPondering(std::stod(option.value) / 1000.0);
				return true;
			}
			if (option.name == "max_memory")
			{
				if (std::stod(option.value) == 0)
					setMaxMemory(0.8 * ml::Device::cpu().memory() * 1024 * 1024);
				else
					setMaxMemory(std::stoll(option.value));
				return true;
			}
			if (option.name == "rows")
			{
				setRows(std::stod(option.value));
				return true;
			}
			if (option.name == "cols")
			{
				setCols(std::stod(option.value));
				return true;
			}
			if (option.name == "rules")
			{
				setRules(rulesFromString(option.value));
				return true;
			}
			if (option.name == "folder")
				return true;

		} catch (std::exception &e)
		{
		}
		return false;
	}
} /* namespace ag */

