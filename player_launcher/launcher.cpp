/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <iostream>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <filesystem>

using namespace ag;

#ifdef _WIN32
const std::string path_separator = "\\";
#elif __linux__
const std::string path_separator = "/";
#endif

Json createDefaultConfig()
{
	Json result;
	result["protocol"] = toString(ProtocolType::GOMOCUP);
	result["use_logging"] = false;
	result["always_ponder"] = false;
	result["swap2_openings_file"] = "swap2_openings.txt";
	result["networks"] = { { "freestyle", "freestyle_10x128.bin" }, { "standard", "standard_10x128.bin" }, { "renju", "" }, { "caro", "" } };
	result["use_symmetries"] = false;
	result["threads"][0] = Json( { { "device", "CPU" } });
	result["search_options"] = SearchConfig::getDefault();
	result["tree_options"] = TreeConfig::getDefault();
	result["cache_options"] = CacheConfig::getDefault();
	return result;
}
Json loadConfig(const std::string &localLaunchPath)
{
	if (std::filesystem::exists(localLaunchPath + "config.json"))
	{
		FileLoader fl(localLaunchPath + "config.json");
		return fl.getJson();
	}
	else
	{
		Json config = createDefaultConfig();
		FileSaver fs(localLaunchPath + "config.json");
		fs.save(config, SerializedObject(), 2);
		return config;
	}
}

int main(int argc, char *argv[])
{
	// extract local launch path
	std::string localLaunchPath(argv[0]);
	int last_slash = -1;
	for (int i = 0; i < (int) localLaunchPath.length(); i++)
		if (localLaunchPath[i] == path_separator[0])
			last_slash = i + 1;
	localLaunchPath = localLaunchPath.substr(0, last_slash);

	// load configuration
	Json config = loadConfig(localLaunchPath);
	config["swap2_openings_file"] = localLaunchPath + path_separator + static_cast<std::string>(config["swap2_openings_file"]);
	config["networks"]["freestyle"] = localLaunchPath + "networks" + path_separator + static_cast<std::string>(config["networks"]["freestyle"]);
	config["networks"]["standard"] = localLaunchPath + "networks" + path_separator + static_cast<std::string>(config["networks"]["standard"]);

	std::cout << config.dump(2);

	// setup logging if enabled
	std::unique_ptr<std::ofstream> logfile;
	if (static_cast<bool>(config["use_logging"]))
	{
		logfile = std::make_unique<std::ofstream>(localLaunchPath + path_separator + "logs" + path_separator + currentDateTime() + ".log");
		Logger::enable();
		Logger::redirectTo(*logfile);
	}
	Logger::write(std::string("Compiled on ") + __DATE__ + " at " + __TIME__ + '\n');
	Logger::write("Launched in " + localLaunchPath + '\n');

	GameConfig game_config(GameRules::FREESTYLE, 20, 20);
	SearchEngine engine(game_config, config);
//	GomocupPlayer player(localLaunchPath);
//	player.run();

	return 0;
}

