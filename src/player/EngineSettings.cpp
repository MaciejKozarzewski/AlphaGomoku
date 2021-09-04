/*
 * EngineSettings.cpp
 *
 *  Created on: Apr 17, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <libml/hardware/Device.hpp>

#include <algorithm>

namespace ag
{
	EngineSettings::EngineSettings() :
			game_config(GameRules::FREESTYLE, 0)
	{
	}
	void EngineSettings::setRows(int rows) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.rows = rows;
	}
	void EngineSettings::setCols(int cols) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.cols = cols;
	}
	void EngineSettings::setRules(GameRules rules) noexcept
	{
		std::lock_guard lock(mutex);
		game_config.rules = rules;
	}
	GameConfig EngineSettings::getGameConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return game_config;
	}

	void EngineSettings::setSearchStartTime(double t) noexcept
	{
		std::lock_guard lock(mutex);
		search_start_time = t;
	}
	double EngineSettings::getElapsedTime() const noexcept
	{
		std::lock_guard lock(mutex);
		return getTime() - search_start_time;
	}
	uint64_t EngineSettings::getMaxMemory() const noexcept
	{
		std::lock_guard lock(mutex);
		return max_memory;
	}
	double EngineSettings::getTimeForTurn() const noexcept
	{
		std::lock_guard lock(mutex);
		if (time_for_turn == 0) // timeout turn set to 0 - play as fast as possible
			return 0.0;
		else
		{
			if (time_for_match <= 0) // there is no match time limit
				return time_for_turn - PROTOCOL_OVERHEAD; // just use timeout turn
			else
				return std::min(time_for_turn, (TIME_FRACTION * time_left)) - PROTOCOL_OVERHEAD;
		}
	}
	double EngineSettings::getTimeForSwap2(int stones) const noexcept
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
	double EngineSettings::getTimeForPondering() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_for_pondering;
	}

	void EngineSettings::setMaxMemory(uint64_t m) noexcept
	{
		std::lock_guard lock(mutex);
		max_memory = m;
	}
	void EngineSettings::setTimeForMatch(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_match = t;
	}
	void EngineSettings::setTimeForTurn(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_turn = t;
	}
	void EngineSettings::setTimeLeft(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_left = t;
	}
	void EngineSettings::setTimeForPondering(double t) noexcept
	{
		std::lock_guard lock(mutex);
		time_for_pondering = t;
	}

	bool EngineSettings::setOption(const Option &option) noexcept
	{
		try
		{
			if (option.name == "time_for_turn")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					setTimeForTurn(max_double_value);
				else
					setTimeForTurn(static_cast<double>(value) / 1000.0);
				return true;
			}
			if (option.name == "time_for_match")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					setTimeForMatch(max_double_value);
				else
					setTimeForMatch(static_cast<double>(value) / 1000.0);
				return true;
			}
			if (option.name == "time_left")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					setTimeLeft(max_double_value);
				else
					setTimeLeft(static_cast<double>(value) / 1000.0);
				return true;
			}
			if (option.name == "time_for_pondering")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					setTimeForPondering(max_double_value);
				else
					setTimeForPondering(static_cast<double>(value) / 1000.0);
				return true;
			}
			if (option.name == "max_memory")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					setMaxMemory(0.8 * ml::Device::cpu().memory() * 1024 * 1024);
				else
					setMaxMemory(value);
				return true;
			}
			if (option.name == "rows")
			{
				setRows(std::stoi(option.value));
				return true;
			}
			if (option.name == "cols")
			{
				setCols(std::stoi(option.value));
				return true;
			}
			if (option.name == "rules")
			{
				setRules(rulesFromString(option.value));
				return true;
			}
			if (option.name == "folder")
				return true; // engine does not make use of any storage for temporary data

		} catch (std::exception &e)
		{
			Logger::write(e.what());
		}
		return false;
	}
} /* namespace ag */

