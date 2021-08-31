/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/version.hpp>

#include "engine_manager.hpp"

#include <filesystem>
#include <iostream>
#include <future>

using namespace ag;

//#ifdef _WIN32
//const std::string path_separator = "\\";
//#elif __linux__
//const std::string path_separator = "/";
//#endif

//ArgumentParser arg_parser;
//InputListener input_listener(std::cin);
//OutputSender output_sender(std::cout);
//
//MessageQueue input_queue;
//MessageQueue output_queue;
//std::unique_ptr<Protocol> protocol;

//void print_help()
//{
//	std::cout << arg_parser.getHelpMessage() << std::endl;
//	exit(0);
//}
//void print_version()
//{
//	std::cout << arg_parser.getExecutablename() << " (" << ProgramInfo::name() << ") " << ProgramInfo::version() << std::endl;
//	std::cout << ProgramInfo::copyright() << std::endl;
//	std::cout << ProgramInfo::build() << std::endl;
//	std::cout << ProgramInfo::license() << std::endl;
//	std::cout << "For more information visit " << ProgramInfo::website() << std::endl;
//	exit(0);
//}
//void set_protocol(const std::string &name)
//{
//	if (name == "gomocup")
//	{
//		protocol = std::make_unique<GomocupProtocol>(input_queue, output_queue);
//		return;
//	}
//	if (name == "yixinboard")
//	{
//		throw std::logic_error("not implemented");
//	}
//	if (name == "ugi")
//	{
//		throw std::logic_error("not implemented");
//	}
//	throw std::logic_error("unknown protocol '" + name + "'");
//}
//void create_argument_parser()
//{
//	arg_parser.addArgument("--help", "-h").help("display this help text and exit").action(asdf::asd);
//	arg_parser.addArgument("--version", "-v").help("display version information and exit").action(print_version);
//	arg_parser.addArgument("--protocol", "-p").help(
//			"sets default protocol used by the engine. <value> can be one of: gomocup, yixinboard, ugi. If not specified, program will use gomocup protocol.").action(
//			set_protocol);
//	arg_parser.addArgument("--benchmark", "-b");
//	arg_parser.addArgument("--configure").help(
//			"runs configuration mode and exits. Resulting configuration file is saved as <value> in path relative to the executable.");
//	arg_parser.addArgument("--load-config").help(
//			"loads config named <value> with path relative to the executable. If not specified, program will load file \"config.json\".").action(
//			[](const std::string &arg)
//			{});
//}

int main(int argc, char *argv[])
{
	EngineManager engine_manager;
	engine_manager.processArguments(argc, argv);
	std::cout << "END\n";

//	set_protocol("gomocup"); // default protocol
//
//	if (argc > 1)
//	{
//		if (std::string(argv[1]) == "configure")
//			createConfig(arg_parser.getLaunchPath());
//		exit(0);
//	}
//
//	Json config = loadConfig(arg_parser.getLaunchPath());
//	if (config.isNull())
//		exit(0);
//	config["swap2_openings_file"] = arg_parser.getLaunchPath() + static_cast<std::string>(config["swap2_openings_file"]);
//	config["networks"]["freestyle"] = arg_parser.getLaunchPath() + "networks" + path_separator
//			+ static_cast<std::string>(config["networks"]["freestyle"]);
//	config["networks"]["standard"] = arg_parser.getLaunchPath() + "networks" + path_separator
//			+ static_cast<std::string>(config["networks"]["standard"]);
//
//	std::ofstream logfile;
//
//	GameConfig game_config;
//	ResourceManager resource_manager;
//
//	std::unique_ptr<SearchEngine> search_engine;
//
//	std::future<void> input_future;
//	std::future<void> search_future;
//	bool is_running = true;
//	while (is_running)
//	{
//		if (input_future.valid())
//		{
//			std::future_status input_status = input_future.wait_for(std::chrono::milliseconds(10));
//			if (input_status == std::future_status::ready) // there is some input to process
//				input_future.get();
//		}
//		else
//		{
//			input_future = std::async(std::launch::async, [&]()
//			{	protocol->processInput(input_listener);});
//		}
//
//		while (input_queue.isEmpty() == false)
//		{
//			Message input_message = input_queue.pop();
//			switch (input_message.getType())
//			{
//				case MessageType::START_PROGRAM:
//				{
//					// it does not actually create search engine, only initializes
//					if (static_cast<bool>(config["use_logging"])) // setup logging to file if enabled
//					{
//						logfile = std::ofstream(arg_parser.getLaunchPath() + path_separator + "logs" + path_separator + currentDateTime() + ".log");
//						Logger::enable();
//						Logger::redirectTo(logfile);
//					}
//					break;
//				}
//				case MessageType::SET_OPTION:
//				{
//					bool success = resource_manager.setOption(input_message.getOption());
//					if (success == false)
//						output_queue.push(Message(MessageType::ERROR, std::string("unknown option ") + input_message.getOption().name));
//					break;
//				}
//				case MessageType::SET_POSITION:
//				{
//					resource_manager.setSearchStartTime(getTime());
//					if (search_engine == nullptr) // search engine is only initialized here
//						search_engine = std::make_unique<SearchEngine>(config, resource_manager);
//
//					search_engine->stopSearch();
//					if (search_future.valid())
//						search_future.get(); // wait for search task to end
//					search_engine->setPosition(input_message.getListOfMoves());
//					break;
//				}
//				case MessageType::START_SEARCH:	// START_SEARCH must be preceded with SET_POSITION message
//				{
//					if (search_engine == nullptr)
//					{
//						output_queue.push(Message(MessageType::ERROR, "search engine was not initialized"));
//						break;
//					}
//					if (search_future.valid() == true)
//					{
//						output_queue.push(Message(MessageType::ERROR, "search engine is already running"));
//						break;
//					}
//					if (input_message.getString() == "bestmove")
//					{
//						search_future = std::async(std::launch::async, [&]()
//						{
//							Message msg = search_engine->makeMove();
//							output_queue.push(search_engine->getSearchSummary());
//							output_queue.push(msg);
//							if (static_cast<bool>(config["always_ponder"]))
//							{
//								resource_manager.setSearchStartTime(getTime());
//								resource_manager.setTimeForPondering(2147483647.0);
//								search_engine->setPosition(msg.getMove());
//								output_queue.push(search_engine->ponder());
//								output_queue.push(search_engine->getSearchSummary());
//							}
//						});
//					}
//					if (input_message.getString() == "swap2")
//					{
//						search_future = std::async(std::launch::async, [&]()
//						{
//							Message msg = search_engine->swap2();
//							output_queue.push(search_engine->getSearchSummary());
//							output_queue.push(msg);
//						});
//					}
//					if (input_message.getString() == "ponder")
//					{
//						resource_manager.setSearchStartTime(getTime()); // pondering does not pay cost of initialization
//						search_future = std::async(std::launch::async, [&]()
//						{
//							Message msg = search_engine->ponder();
//							output_queue.push(search_engine->getSearchSummary());
//							output_queue.push(msg);
//						});
//					}
//					break;
//				}
//				case MessageType::STOP_SEARCH:
//				{
//					if (search_engine != nullptr)
//						search_engine->stopSearch();
//					break;
//				}
//				case MessageType::BEST_MOVE:
//					break; // should never receive this
//				case MessageType::EXIT_PROGRAM:
//				{
//					is_running = false;
//					break;
//				}
//				case MessageType::EMPTY_MESSAGE:
//				case MessageType::PLAIN_STRING:
//				case MessageType::UNKNOWN_COMMAND:
//				case MessageType::ERROR:
//				case MessageType::INFO_MESSAGE:
//				case MessageType::ABOUT_ENGINE:
//					break; // should never receive this
//			}
//		}
//
//		if (search_future.valid())
//		{
//			std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
//			if (search_status == std::future_status::ready)
//				search_future.get();
//		}
//
//		protocol->processOutput(output_sender);
//	}

	return 0;
}

