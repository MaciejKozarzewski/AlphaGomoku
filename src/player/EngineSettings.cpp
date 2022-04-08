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
#include <alphagomoku/utils/file_util.hpp>

#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>

#include <algorithm>
#include <filesystem>

namespace
{
	ag::Move json_to_move(const Json &json)
	{
		return ag::Move(json["row"].getInt(), json["col"].getInt(), ag::signFromString(json["sign"].getString()));
	}
	std::vector<std::vector<ag::Move>> load_opening_book(const std::string &path)
	{
		if (std::filesystem::exists(path) == false)
			throw std::logic_error("No swap2 opening book");
		else
		{
			ag::FileLoader fl(path);
			const Json &json = fl.getJson();

			std::vector<std::vector<ag::Move>> result;
			for (int i = 0; i < json.size(); i++)
			{
				std::vector<ag::Move> tmp;
				for (int j = 0; j < json[i].size(); j++)
					tmp.push_back(json_to_move(json[i][j]));
				result.push_back(tmp);
			}
			return result;
		}
	}
}

namespace ag
{
	EngineSettings::EngineSettings(const Json &config) :
			swap2_openings(load_opening_book(config["swap2_openings_file"].getString())),
			game_config(GameRules::FREESTYLE, 0, 0),
			tree_config(config["tree_options"]),
			search_config(config["search_options"]),
			thread_num(config["search_threads"].getInt()),
			auto_pondering(config["always_ponder"].getBool())
	{
		const Json &device_configuration = config["devices"];
		for (int i = 0; i < device_configuration.size(); i++)
			device_configs.push_back(DeviceConfig(device_configuration[i]));

		path_to_networks.insert( { GameRules::FREESTYLE, config["networks"]["freestyle"].getString() });
		path_to_networks.insert( { GameRules::STANDARD, config["networks"]["standard"].getString() });
		path_to_networks.insert( { GameRules::RENJU, config["networks"]["renju"].getString() });
		path_to_networks.insert( { GameRules::CARO, config["networks"]["caro"].getString() });
	}

	SetOptionOutcome EngineSettings::setOption(const Option &option) noexcept
	{
		std::lock_guard lock(mutex);
		try
		{
			if (option.name == "rows")
			{
				const int tmp = game_config.rows;
				game_config.rows = std::stoi(option.value);
				return (tmp == game_config.rows) ? SetOptionOutcome::SUCCESS : SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "columns")
			{
				const int tmp = game_config.cols;
				game_config.cols = std::stoi(option.value);
				return (tmp == game_config.cols) ? SetOptionOutcome::SUCCESS : SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "rules")
			{
				const GameRules tmp = game_config.rules;
				game_config.rules = rulesFromString(option.value);
				return (tmp == game_config.rules) ? SetOptionOutcome::SUCCESS : SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE;
			}
			if (option.name == "time_for_match")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					time_for_match = max_double_value;
				else
					time_for_match = value / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_left")
			{
				time_left = std::stoll(option.value) / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_for_turn")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					time_for_turn = max_double_value;
				else
					time_for_turn = value / 1000.0;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_increment")
			{
				time_increment = std::max(0.0, std::stoll(option.value) / 1000.0); // time increment must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "protocol_lag")
			{
				protocol_lag = std::max(0.0, std::stoll(option.value) / 1000.0); // protocol lag must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_depth")
			{
				int64_t value = std::stoll(option.value);
				if (value < 0)
					max_depth = max_int_value;
				else
					max_depth = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_nodes")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					max_nodes = max_int_value;
				else
					max_nodes = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "thread_num")
			{
				thread_num = std::stoll(option.value); // thread_num must not be negative
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "max_memory")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					max_memory = 0.75 * ml::Device::cpu().memory() * 1024 * 1024;
				else
					max_memory = value;
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "style")
			{
				search_config.style_factor = std::stoi(option.value);
				return SetOptionOutcome::SUCCESS;
			}

			if (option.name == "analysis_mode")
			{
				analysis_mode = static_cast<bool>(std::stoi(option.value));
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "auto_pondering")
			{
				auto_pondering = static_cast<bool>(std::stoi(option.value));
				return SetOptionOutcome::SUCCESS;
			}
			if (option.name == "time_for_pondering")
			{
				int64_t value = std::stoll(option.value);
				if (value <= 0)
					time_for_pondering = max_double_value;
				else
					time_for_pondering = value / 1000.0;
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
	const std::vector<std::vector<Move>>& EngineSettings::getSwap2Openings() const
	{
		return swap2_openings;
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
		return max_memory - 150 * 1024 * 1024; // assuming 150MB offset for all objects other than the Tree
	}
	float EngineSettings::getStyleFactor() const noexcept
	{
		std::lock_guard lock(mutex);
		return 0.25f * static_cast<int>(search_config.style_factor);
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

