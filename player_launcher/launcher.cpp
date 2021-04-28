/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <iostream>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <filesystem>
#include <future>

using namespace ag;

#ifdef _WIN32
const std::string path_separator = "\\";
#elif __linux__
const std::string path_separator = "/";
#endif

Json createDefaultConfig()
{
	Json result;
	result["use_logging"] = false;
	result["always_ponder"] = false;
	result["swap2_openings_file"] = "swap2_openings.json";
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

	// setup logging if enabled
	std::ofstream logfile;
	if (static_cast<bool>(config["use_logging"]))
	{
		logfile = std::ofstream(localLaunchPath + path_separator + "logs" + path_separator + currentDateTime() + ".log");
		Logger::enable();
		Logger::redirectTo(logfile);
	}
	Logger::write(std::string("Compiled on ") + __DATE__ + " at " + __TIME__);
	Logger::write("Launched in " + localLaunchPath);

	GameConfig game_config;
	ResourceManager resource_manager;

	InputListener input_listener(std::cin);
	OutputSender output_sender(std::cout);

	MessageQueue input_queue;
	MessageQueue output_queue;
	GomocupProtocol protocol(input_queue, output_queue);

	std::unique_ptr<SearchEngine> search_engine;
	bool is_running = true;
	std::future<void> input_future;
	std::future<Message> search_future;
	while (is_running)
	{
		if (input_future.valid())
		{
			std::future_status input_status = input_future.wait_for(std::chrono::milliseconds(10));
			if (input_status == std::future_status::ready) //  there is some input to process
				input_future.get();
		}
		else
		{
			input_future = std::async(std::launch::async, [&]()
			{	protocol.processInput(input_listener);});
		}

		while (input_queue.isEmpty() == false)
		{
			Message msg = input_queue.pop();
			switch (msg.getType())
			{
				case MessageType::CHANGE_PROTOCOL:
				case MessageType::START_PROGRAM:
					break;
				case MessageType::SET_OPTION:
				{
					if (msg.isEmpty() == false)
					{
						bool success = resource_manager.setOption(msg.getOption());
						if (success == false)
							output_queue.push(Message(MessageType::ERROR, std::string("unknown option ") + msg.getOption().name));
					}
					break;
				}
				case MessageType::SET_POSITION:
				{
					if (search_engine == nullptr)
						search_engine = std::make_unique<SearchEngine>(config, resource_manager);
					search_engine->setPosition(msg.getListOfMoves());
					break;
				}
				case MessageType::START_SEARCH:
				{
					if (search_engine == nullptr)
						search_engine = std::make_unique<SearchEngine>(config, resource_manager);
					resource_manager.setSearchStartTime(getTime());
					search_engine->stopSearch();
					if (msg.getString() == "bestmove")
					{
						search_future = std::async(std::launch::async, [&]()
						{	return search_engine->makeMove();});
					}
					if (msg.getString() == "swap2")
					{
						search_future = std::async(std::launch::async, [&]()
						{	return search_engine->swap2();});
					}
					if (msg.getString() == "ponder")
					{
						search_future = std::async(std::launch::async, [&]()
						{	return search_engine->ponder();});
					}
					break;
				}
				case MessageType::STOP_SEARCH:
				{
					if (search_engine == nullptr)
						search_engine = std::make_unique<SearchEngine>(config, resource_manager);
					search_engine->stopSearch();
					break;
				}
				case MessageType::MAKE_MOVE:
					break; // should never receive this
				case MessageType::EXIT_PROGRAM:
				{
					is_running = false;
					break;
				}
				case MessageType::EMPTY_MESSAGE:
				case MessageType::PLAIN_STRING:
				case MessageType::UNKNOWN_COMMAND:
				case MessageType::ERROR:
				case MessageType::INFO_MESSAGE:
				case MessageType::ABOUT_ENGINE:
					break; // should never receive this
			}
			Logger::write("Input Message : " + msg.info());
		}

		if (search_future.valid())
		{
			std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
			if (search_status == std::future_status::ready)
			{
				Message search_msg = search_future.get();
				Logger::write("Output Message : " + search_msg.info());
				output_queue.push(search_engine->getSearchSummary());
				output_queue.push(search_msg);
//				if (static_cast<bool>(config["always_ponder"]))
//				{
//					input_listener.pushLine("PONDER");
//					protocol.processInput(input_listener);
//				}
			}
		}

		protocol.processOutput(output_sender);
	}

	return 0;
}

