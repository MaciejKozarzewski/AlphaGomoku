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
	std::string toString(EngineStyle es)
	{
		switch (es)
		{
			case EngineStyle::DEFENSIVE:
				return "DEFENSIVE";
			case EngineStyle::CLASSIC:
				return "CLASSIC";
			case EngineStyle::OFFENSIVE:
				return "OFFENSIVE";
			default:
				return "unknown style";
		}
	}
	EngineStyle engineStyleFromString(const std::string &str)
	{
		if (str == "DEFENSIVE" or str == "defensive")
			return EngineStyle::DEFENSIVE;
		if (str == "CLASSIC" or str == "classic")
			return EngineStyle::CLASSIC;
		if (str == "OFFENSIVE" or str == "offensive")
			return EngineStyle::OFFENSIVE;
		throw std::logic_error("unknown style '" + str + "'");
	}

	EngineSettings::EngineSettings() :
			game_config(GameRules::FREESTYLE, 0, 0)
	{
	}
//	void EngineSettings::setSearchStartTime(double t) noexcept
//	{
//		std::lock_guard lock(mutex);
//		search_start_time = t;
//	}
//	double EngineSettings::getElapsedTime() const noexcept
//	{
//		std::lock_guard lock(mutex);
//		return getTime() - search_start_time;
//	}
//	uint64_t EngineSettings::getMaxMemory() const noexcept
//	{
//		std::lock_guard lock(mutex);
//		return max_memory;
//	}
//	double EngineSettings::getTimeForTurn() const noexcept
//	{
//		std::lock_guard lock(mutex);
//		if (time_for_turn == 0) // timeout turn set to 0 - play as fast as possible
//			return 0.0;
//		else
//		{
//			if (time_for_match <= 0) // there is no match time limit
//				return time_for_turn - PROTOCOL_OVERHEAD; // just use timeout turn
//			else
//				return std::min(time_for_turn, (TIME_FRACTION * time_left)) - PROTOCOL_OVERHEAD;
//		}
//	}
//	double EngineSettings::getTimeForSwap2(int stones) const noexcept
//	{
//		std::lock_guard lock(mutex);
//		switch (stones)
//		{
//			case 0:
//				return 0.0;
//			case 3:
//				return SWAP2_FRACTION * time_left;
//			case 5:
//				return TIME_FRACTION * time_left;
//			default:
//				return 0.0;
//		}
//	}
//	double EngineSettings::getTimeForPondering() const noexcept
//	{
//		std::lock_guard lock(mutex);
//		return time_for_pondering;
//	}

	bool EngineSettings::setOption(const Option &option) noexcept
	{
		std::lock_guard lock(mutex);
		try
		{
			if (option.name == "rows")
			{
				this->game_config.rows = std::stoi(option.value);
				return true;
			}
			if (option.name == "columns")
			{
				this->game_config.cols = std::stoi(option.value);
				return true;
			}
			if (option.name == "rules")
			{
				this->game_config.rules = rulesFromString(option.value);
				return true;
			}
			if (option.name == "time_for_match")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->time_for_match = max_double_value;
				else
					this->time_for_match = value / 1000.0;
				return true;
			}
			if (option.name == "time_left")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					this->time_left = max_double_value;
				else
					this->time_left = value / 1000.0;
				return true;
			}
			if (option.name == "time_for_turn")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					this->time_for_turn = max_double_value;
				else
					this->time_for_turn = value / 1000.0;
				return true;
			}
			if (option.name == "time_increment")
			{
				this->time_increment = std::max(0.0, std::stoll(option.value) / 1000.0); // time increment must not be negative
				return true;
			}
			if (option.name == "protocol_lag")
			{
				this->protocol_lag = std::max(0.0, std::stoll(option.value) / 1000.0); // protocol lag must not be negative
				return true;
			}
			if (option.name == "max_depth")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->max_depth = max_int_value;
				else
					this->max_depth = value;
				return true;
			}
			if (option.name == "max_nodes")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->max_nodes = max_int_value;
				else
					this->max_nodes = value;
				return true;
			}
			if (option.name == "thread_num")
			{
				this->thread_num = std::stoll(option.value); // thread_num must not be negative
				return true;
			}
			if (option.name == "max_memory")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->max_memory = 0.75 * ml::Device::cpu().memory() * 1024 * 1024;
				else
					this->max_memory = value;
				return true;
			}
			if (option.name == "style")
			{
				this->style = engineStyleFromString(option.value);
				return true;
			}

			if (option.name == "analysis_mode")
			{
				this->analysis_mode = static_cast<bool>(std::stoi(option.value));
				return true;
			}
			if (option.name == "auto_pondering")
			{
				this->auto_pondering = static_cast<bool>(std::stoi(option.value));
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

	const GameConfig& EngineSettings::getGameConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return game_config;
	}
	const TreeConfig& EngineSettings::getTreeConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return tree_config;
	}
	const CacheConfig& EngineSettings::getCacheConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return cache_config;
	}
	const SearchConfig& EngineSettings::getSearchConfig() const noexcept
	{
		std::lock_guard lock(mutex);
		return search_config;
	}
	double EngineSettings::getTimeForMatch() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_for_match;
	}
	double EngineSettings::getTimeLeft() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_left;
	}
	double EngineSettings::getTimeForTurn() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_for_turn;
	}
	double EngineSettings::getTimeIncrement() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_increment;
	}
	double EngineSettings::getProtocolLag() const noexcept
	{
		std::lock_guard lock(mutex);
		return protocol_lag;
	}
	int EngineSettings::getMaxDepth() const noexcept
	{
		std::lock_guard lock(mutex);
		return max_depth;
	}
	int EngineSettings::getMaxNodes() const noexcept
	{
		std::lock_guard lock(mutex);
		return max_nodes;
	}
	int EngineSettings::getThreadNum() const noexcept
	{
		std::lock_guard lock(mutex);
		return thread_num;
	}
	int64_t EngineSettings::getMaxMemory() const noexcept
	{
		std::lock_guard lock(mutex);
		return max_memory;
	}
	EngineStyle EngineSettings::getStyle() const noexcept
	{
		std::lock_guard lock(mutex);
		return style;
	}
	bool EngineSettings::isInAnalysisMode() const noexcept
	{
		std::lock_guard lock(mutex);
		return analysis_mode;
	}
	bool EngineSettings::isUsingAutoPondering() const noexcept
	{
		std::lock_guard lock(mutex);
		return auto_pondering;
	}
	bool EngineSettings::isUsingSymmetries() const noexcept
	{
		std::lock_guard lock(mutex);
		return use_symmetries;
	}
} /* namespace ag */

