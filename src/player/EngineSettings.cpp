/*
 * EngineSettings.cpp
 *
 *  Created on: Apr 17, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>

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

	EngineSettings::EngineSettings(const Json &config) :
			game_config(GameRules::FREESTYLE, 0, 0),
			tree_config(config["tree_options"]),
			search_config(config["search_options"]),
			thread_num(static_cast<int>(config["search_threads"])),
			auto_pondering(static_cast<bool>(config["always_ponder"]))
	{
		const Json &device_configuration = config["devices"];
		for (int i = 0; i < device_configuration.size(); i++)
			device_configs.push_back(DeviceConfig(device_configuration[i]));

		path_to_networks.insert( { GameRules::FREESTYLE, static_cast<std::string>(config["networks"]["freestyle"]) });
		path_to_networks.insert( { GameRules::STANDARD, static_cast<std::string>(config["networks"]["standard"]) });
		path_to_networks.insert( { GameRules::RENJU, static_cast<std::string>(config["networks"]["renju"]) });
		path_to_networks.insert( { GameRules::CARO, static_cast<std::string>(config["networks"]["caro"]) });
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

	SetOptionOutcome EngineSettings::setOption(const Option &option) noexcept
	{
		std::lock_guard lock(mutex);
		try
		{
			if (option.name == "rows")
			{
				this->game_config.rows = std::stoi(option.value);
				return SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "columns")
			{
				this->game_config.cols = std::stoi(option.value);
				return SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "rules")
			{
				this->game_config.rules = rulesFromString(option.value);
				return SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "time_for_match")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->time_for_match = max_double_value;
				else
					this->time_for_match = value / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_left")
			{
				this->time_left = std::stoll(option.value) / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_for_turn")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					this->time_for_turn = max_double_value;
				else
					this->time_for_turn = value / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_increment")
			{
				this->time_increment = std::max(0.0, std::stoll(option.value) / 1000.0); // time increment must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "protocol_lag")
			{
				this->protocol_lag = std::max(0.0, std::stoll(option.value) / 1000.0); // protocol lag must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_depth")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					this->max_depth = max_int_value;
				else
					this->max_depth = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_nodes")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->max_nodes = max_int_value;
				else
					this->max_nodes = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "thread_num")
			{
				this->thread_num = std::stoll(option.value); // thread_num must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_memory")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->max_memory = 0.75 * ml::Device::cpu().memory() * 1024 * 1024;
				else
					this->max_memory = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "style")
			{
				search_config.style_factor = static_cast<int>(engineStyleFromString(option.value));
				return SetOptionOutcome::SUCCESS;
			}

			if (option.name == "analysis_mode")
			{
				this->analysis_mode = static_cast<bool>(std::stoi(option.value));
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "auto_pondering")
			{
				this->auto_pondering = static_cast<bool>(std::stoi(option.value));
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_for_pondering")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					this->time_for_pondering = max_double_value;
				else
					this->time_for_pondering = value / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "folder")
				return SetOptionOutcome::SUCCESS; // engine does not make use of any storage for temporary data

		} catch (std::exception &e)
		{
			Logger::write(e.what());
		}
		return SetOptionOutcome::FAILURE;
	}

	std::string EngineSettings::getPathToNetwork() const
	{
		return path_to_networks.find(game_config.rules)->second;
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
	double EngineSettings::getTimeForPondering() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_for_pondering;
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
	float EngineSettings::getStyleFactor() const noexcept
	{
		std::lock_guard lock(mutex);
		return 0.5f * static_cast<int>(search_config.style_factor);
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

	const std::vector<DeviceConfig>& EngineSettings::getDeviceConfigs() const noexcept
	{
		return device_configs;
	}
} /* namespace ag */

