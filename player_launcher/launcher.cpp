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
#include <libml/hardware/Device.hpp>

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
	result["networks"]["freestyle"] = "freestyle_10x128.bin";
	result["networks"]["standard"] = "standard_10x128.bin";
	result["networks"]["renju"] = "";
	result["networks"]["caro"] = "";
	result["use_symmetries"] = true;
	result["threads"][0] = Json( { { "device", "CPU" } });
	result["search_options"]["batch_size"] = 4;
	result["search_options"]["exploration_constant"] = 1.25;
	result["search_options"]["expansion_prior_treshold"] = 1.0e-4;
	result["search_options"]["max_children"] = 30;
	result["search_options"]["noise_weight"] = 0.0;
	result["search_options"]["use_endgame_solver"] = true;
	result["search_options"]["use_vcf_solver"] = true;
	result["tree_options"]["max_number_of_nodes"] = 50000000;
	result["tree_options"]["bucket_size"] = 100000;
	result["cache_options"]["min_cache_size"] = 1048576;
	result["cache_options"]["max_cache_size"] = 1048576;
	result["cache_options"]["update_from_search"] = false;
	result["cache_options"]["update_visit_treshold"] = 1000;
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

	std::ofstream logfile;

	GameConfig game_config;
	ResourceManager resource_manager;

	InputListener input_listener(std::cin);
	OutputSender output_sender(std::cout);

	MessageQueue input_queue;
	MessageQueue output_queue;
	GomocupProtocol protocol(input_queue, output_queue);

	std::unique_ptr<SearchEngine> search_engine;

	std::future<void> input_future;
	std::future<void> search_future;
	bool is_running = true;
	while (is_running)
	{
		if (input_future.valid())
		{
			std::future_status input_status = input_future.wait_for(std::chrono::milliseconds(10));
			if (input_status == std::future_status::ready) // there is some input to process
				input_future.get();
		}
		else
		{
			input_future = std::async(std::launch::async, [&]()
			{	protocol.processInput(input_listener);});
		}

		while (input_queue.isEmpty() == false)
		{
			Message input_message = input_queue.pop();
			switch (input_message.getType())
			{
				case MessageType::CHANGE_PROTOCOL:
				case MessageType::START_PROGRAM:
				{
					if (static_cast<bool>(config["use_logging"])) // setup logging to file if enabled
					{
						logfile = std::ofstream(localLaunchPath + path_separator + "logs" + path_separator + currentDateTime() + ".log");
						Logger::enable();
						Logger::redirectTo(logfile);
					}
					Logger::write(std::string("Compiled on ") + __DATE__ + " at " + __TIME__);
					Logger::write("Launched in " + localLaunchPath);
					break;
				}
				case MessageType::SET_OPTION:
				{
					bool success = resource_manager.setOption(input_message.getOption());
					if (success == false)
						output_queue.push(Message(MessageType::ERROR, std::string("unknown option ") + input_message.getOption().name));
					break;
				}
				case MessageType::SET_POSITION:
				{
					resource_manager.setSearchStartTime(getTime());
					if (search_engine == nullptr) // search engine is only initialized here
						search_engine = std::make_unique<SearchEngine>(config, resource_manager);

					search_engine->stopSearch();
					if (search_future.valid())
						search_future.get(); // wait for search task to end
					search_engine->setPosition(input_message.getListOfMoves());
					break;
				}
				case MessageType::START_SEARCH:	// START_SEARCH must be preceded with SET_POSITION message
				{
					if (search_engine == nullptr)
					{
						output_queue.push(Message(MessageType::ERROR, "search engine was not initialized"));
						break;
					}
					if (search_future.valid() == true)
					{
						output_queue.push(Message(MessageType::ERROR, "search engine is already running"));
						break;
					}
					if (input_message.getString() == "bestmove")
					{
						search_future = std::async(std::launch::async, [&]()
						{
							Message msg = search_engine->makeMove();
							output_queue.push(search_engine->getSearchSummary());
							output_queue.push(msg);
							if (static_cast<bool>(config["always_ponder"]))
							{
								resource_manager.setSearchStartTime(getTime());
								resource_manager.setTimeForPondering(-1);
								search_engine->setPosition(msg.getMove());
								output_queue.push(search_engine->ponder());
								output_queue.push(search_engine->getSearchSummary());
							}
						});
					}
					if (input_message.getString() == "swap2")
					{
						search_future = std::async(std::launch::async, [&]()
						{
							Message msg = search_engine->swap2();
							output_queue.push(search_engine->getSearchSummary());
							output_queue.push(msg);
						});
					}
					if (input_message.getString() == "ponder")
					{
						resource_manager.setSearchStartTime(getTime()); // pondering does not pay cost of initialization
						search_future = std::async(std::launch::async, [&]()
						{
							Message msg = search_engine->ponder();
							output_queue.push(search_engine->getSearchSummary());
							output_queue.push(msg);
						});
					}
					break;
				}
				case MessageType::STOP_SEARCH:
				{
					if (search_engine != nullptr)
					{
						search_engine->stopSearch();
						if (search_future.valid())
							search_future.get(); // wait for search task to end
					}
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
		}

		if (search_future.valid())
		{
			std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
			if (search_status == std::future_status::ready)
				search_future.get();
		}

		protocol.processOutput(output_sender);
	}

	return 0;
}

