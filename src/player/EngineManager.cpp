/*
 * EngineManager.cpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */
#include <alphagomoku/player/EngineManager.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>

#include <filesystem>

namespace
{
#ifdef _WIN32
	const std::string path_separator = "\\";
#elif __linux__
	const std::string path_separator = "/";
#endif
}

namespace ag
{

	EngineManager::EngineManager() :
			argument_parser(""),
			input_listener(std::cin),
			output_sender(std::cout)
	{
		create_arguments();
	}

	void EngineManager::processArguments(int argc, char *argv[])
	{
		try
		{
			argument_parser.parseArguments(argc, argv);
		} catch (ParsingError &pe)
		{
			output_sender.send(pe.what());
			output_sender.send("Try '" + argument_parser.getExecutablename() + " --help' for more information.");
			exit(-1);
		}
		if (display_help)
		{
			help();
			exit(0);
		}
		if (display_version)
		{
			version();
			exit(0);
		}
		load_config(argument_parser.getLaunchPath() + name_of_config);
		if (run_configuration)
		{
			configure();
			exit(0);
		}
		if (run_benchmark)
		{
			benchmark();
			exit(0);
		}
	}
	void EngineManager::run()
	{
		setup_protocol();
		while (is_running)
		{
			process_input_from_user();

			while (input_queue.isEmpty() == false)
			{
				Message input_message = input_queue.pop();
				switch (input_message.getType())
				{
					case MessageType::START_PROGRAM:
					{
						if (static_cast<bool>(config["use_logging"]))
							setup_logging(); // FIXME setting up logger in this way makes the initial 'START' command to disappear
						Logger::write("Using config : " + name_of_config);
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
									resource_manager.setTimeForPondering(2147483647.0);
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
							search_engine->stopSearch();
						break;
					}
					case MessageType::EXIT_PROGRAM:
					{
						is_running = false;
						break;
					}
					default:
						break;
				}
			}

			if (search_future.valid())
			{
				std::future_status search_status = search_future.wait_for(std::chrono::milliseconds(0));
				if (search_status == std::future_status::ready)
					search_future.get();
			}

			protocol->processOutput(output_sender);
		}
	}

	void EngineManager::create_arguments()
	{
		argument_parser.addArgument("--help", "-h").help("display this help text and exit").action([this]()
		{	this->display_help = true;});
		argument_parser.addArgument("--version", "-v").help("display version information and exit").action([this]()
		{	this->display_version = true;});
		argument_parser.addArgument("--load-config").help("load configuration file named <value> with path relative to the executable. "
				"If this option is not specified, program will load file \"config.json\".").action([this](const std::string &arg)
		{	this ->name_of_config = arg;});
		argument_parser.addArgument("--configure", "-c").help(
				"run automatic configuration and exit. The result will be saved as \"config.json\". If this file exists it will be overwritten.").action(
				[this]()
				{	this->run_configuration = true;});
		argument_parser.addArgument("--benchmark", "-b").help(
				"test speed of the available hardware, save to file \"benchmark.json\" and exit. If this file exists it will be overwritten.").action(
				[this]()
				{	this->run_benchmark = true;});
	}
	void EngineManager::help() const
	{
		std::string result = argument_parser.getHelpMessage();
		output_sender.send(result);
	}
	void EngineManager::version() const
	{
		std::string result = argument_parser.getExecutablename() + " (" + ProgramInfo::name() + ") " + ProgramInfo::version() + '\n';
		result += ProgramInfo::copyright() + '\n';
		result += ProgramInfo::build() + '\n';
		result += ProgramInfo::license() + '\n';
		result += "For more information visit " + ProgramInfo::website() + '\n';
		output_sender.send(result);
	}
	void EngineManager::benchmark() const
	{
		Json benchmark_result = ag::run_benchmark(config["networks"]["standard"], output_sender);

		FileSaver fs(argument_parser.getLaunchPath() + "benchmark.json");
		fs.save(benchmark_result, SerializedObject(), 4, false);
	}
	void EngineManager::configure()
	{
		output_sender.send("Starting automatic configuration");
		const std::string path_to_benchmark = argument_parser.getLaunchPath() + "benchmark.json";
		if (not std::filesystem::exists(path_to_benchmark))
		{
			output_sender.send(
					"No benchmark file was found, create it by launching " + ProgramInfo::name() + " from command line with parameter '--benchmark'");
			exit(0);
		}

		Json benchmark_results;
		try
		{
			FileLoader fl(path_to_benchmark);
			benchmark_results = fl.getJson();
		} catch (std::exception &e)
		{
			output_sender.send("The benchmark file is invalid for some reason. Try deleting it and creating a new one.");
			exit(-1);
		}

		if (static_cast<std::string>(benchmark_results["version"]) != ProgramInfo::version())
		{
			output_sender.send(
					"Existing benchmark file is for different version, create a new one by launching " + ProgramInfo::name()
							+ " from command line with parameter '--configure'");
			exit(0);
		}
		createConfig(benchmark_results);
	}
	void EngineManager::load_config(const std::string &path)
	{
		if (std::filesystem::exists(path))
		{
			try
			{
				FileLoader fl(path);
				config = fl.getJson();
				prepare_config();
			} catch (std::exception &e)
			{
				output_sender.send("The configuration file is invalid for some reason. Try deleting it and creating a new one.");
				exit(-1);
			}
		}
		else
		{
			output_sender.send("Could not load configuration file.");
			output_sender.send("You can generate new one by launching " + ProgramInfo::name() + " from command line with parameter '--configure'");
			output_sender.send(
					"If you are sure that configuration file exists, it means that the launch path might not have been parsed correctly due to some special characters in it.");
			exit(-1);
		}
	}
	void EngineManager::prepare_config()
	{
		std::string launch_path = argument_parser.getLaunchPath();
		config["swap2_openings_file"] = launch_path + static_cast<std::string>(config["swap2_openings_file"]);
		launch_path += "networks" + path_separator;
		config["networks"]["freestyle"] = launch_path + static_cast<std::string>(config["networks"]["freestyle"]);
		config["networks"]["standard"] = launch_path + static_cast<std::string>(config["networks"]["standard"]);

	}

	void EngineManager::setup_protocol()
	{
		if (not config.contains("protocol"))
		{
			config["protocol"] = "gomocup";
			Logger::write("No 'protocol' field in the configuration, using GOMOCUP protocol");
		}
		switch (protocolFromString(config["protocol"]))
		{
			case ProtocolType::GOMOCUP:
				protocol = std::make_unique<GomocupProtocol>(input_queue, output_queue);
				break;
			case ProtocolType::YIXINBOARD:
				break;
			case ProtocolType::UGI:
				break;
		}
	}
	void EngineManager::process_input_from_user()
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
			{	protocol->processInput(input_listener);});
		}
	}
	void EngineManager::setup_logging()
	{
		logfile = std::ofstream(argument_parser.getLaunchPath() + "logs" + path_separator + currentDateTime() + ".log");
		Logger::enable();
		Logger::redirectTo(logfile);
	}
} /* namespace ag */

