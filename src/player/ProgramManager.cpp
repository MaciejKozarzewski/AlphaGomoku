/*
 * ProgramManager.cpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */
#include <alphagomoku/player/ProgramManager.hpp>
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

	ProgramManager::ProgramManager() :
			argument_parser(""),
			input_listener(std::cin),
			output_sender(std::cout)
	{
		create_arguments();
	}

	bool ProgramManager::processArguments(int argc, char *argv[])
	{
		try
		{
			argument_parser.parseArguments(argc, argv);
		} catch (ParsingError &pe)
		{
			output_sender.send(pe.what());
			output_sender.send("Try '" + argument_parser.getExecutablename() + " --help' for more information.");
			throw;
		}
		if (display_help)
		{
			help();
			return false;
		}
		if (display_version)
		{
			version();
			return false;
		}
		if (run_configuration)
		{
			configure();
			return false;
		}
		bool successfully_loaded_config = load_config(argument_parser.getLaunchPath() + name_of_config_file);
		if (run_benchmark)
		{
			benchmark();
			return false;
		}
		return successfully_loaded_config;
	}
	void ProgramManager::run()
	{
		engine_settings = std::make_unique<EngineSettings>(config);
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
						if (use_logging)
							setup_logging();
						Logger::write("Using config : " + name_of_config_file);
						break;
					}
					case MessageType::SET_OPTION:
					{
						SetOptionOutcome outcome = engine_settings->setOption(input_message.getOption());
						switch (outcome)
						{
							case SetOptionOutcome::SUCCESS:
								break;
							case SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE:
								search_engine = nullptr;
								break;
							case SetOptionOutcome::FAILURE:
								output_queue.push(Message(MessageType::ERROR, std::string("unknown option ") + input_message.getOption().name));
								break;
						}
						break;
					}
					case MessageType::SET_POSITION:
					{
						setPosition(input_message.getListOfMoves());
						break;
					}
					case MessageType::START_SEARCH:
					{
						time_manager.setup();
						time_manager.startTimer();
						if (getEngine().isSearchFinished() == false)
						{
							output_queue.push(Message(MessageType::ERROR, "search engine is already running"));
							break;
						}
						getEngine().setPosition(board, sign_to_move);

						break;
					}
					case MessageType::STOP_SEARCH:
					{
						getEngine().stopSearch();
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
	/*
	 * private
	 */
	void ProgramManager::create_arguments()
	{
		argument_parser.addArgument("--help", "-h").help("display this help text and exit").action([this]()
		{	this->display_help = true;});
		argument_parser.addArgument("--version", "-v").help("display version information and exit").action([this]()
		{	this->display_version = true;});
		argument_parser.addArgument("--load-config").help("load configuration file named <value> with path relative to the executable. "
				"If this option is not specified, program will load file \"config.json\".").action([this](const std::string &arg)
		{	this ->name_of_config_file = arg;});
		argument_parser.addArgument("--configure", "-c").help(
				"run automatic configuration and exit. The result will be saved as \"config.json\". If this file exists it will be overwritten.").action(
				[this]()
				{	this->run_configuration = true;});
		argument_parser.addArgument("--benchmark", "-b").help(
				"test speed of the available hardware, save to file \"benchmark.json\" and exit. If this file exists it will be overwritten.").action(
				[this]()
				{	this->run_benchmark = true;});
	}
	void ProgramManager::help() const
	{
		std::string result = argument_parser.getHelpMessage();
		output_sender.send(result);
	}
	void ProgramManager::version() const
	{
		std::string result = argument_parser.getExecutablename() + " (" + ProgramInfo::name() + ") " + ProgramInfo::version() + '\n';
		result += ProgramInfo::copyright() + '\n';
		result += ProgramInfo::build() + '\n';
		result += ProgramInfo::license() + '\n';
		result += "For more information visit " + ProgramInfo::website() + '\n';
		output_sender.send(result);
	}
	void ProgramManager::benchmark() const
	{
		Json benchmark_result = ag::run_benchmark(config["networks"]["standard"], output_sender);

		FileSaver fs(argument_parser.getLaunchPath() + "benchmark.json");
		fs.save(benchmark_result, SerializedObject(), 4, false);
	}
	void ProgramManager::configure()
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
							+ " from command line with parameter '--benchmark'");
			exit(0);
		}
		createConfig(benchmark_results);
	}
	bool ProgramManager::load_config(const std::string &path)
	{
		if (std::filesystem::exists(path))
		{
			try
			{
				FileLoader fl(path);
				config = fl.getJson();
				prepare_config();
				return true;
			} catch (std::exception &e)
			{
				output_sender.send("The configuration file is invalid for some reason. Try deleting it and creating a new one.");
				throw;
			}
		}
		else
		{
			output_sender.send("Could not load configuration file.");
			output_sender.send("You can generate new one by launching " + ProgramInfo::name() + " from command line with parameter '--configure'");
			output_sender.send(
					"If you are sure that configuration file exists, it means that the launch path was not parsed correctly due to some special characters in it.");
			throw std::runtime_error("file " + path + " not found");
		}
	}
	void ProgramManager::prepare_config()
	{
		std::string launch_path = argument_parser.getLaunchPath();
		config["swap2_openings_file"] = launch_path + static_cast<std::string>(config["swap2_openings_file"]);
		launch_path += "networks" + path_separator;
		config["networks"]["freestyle"] = launch_path + static_cast<std::string>(config["networks"]["freestyle"]);
		config["networks"]["standard"] = launch_path + static_cast<std::string>(config["networks"]["standard"]);

	}

	void ProgramManager::setup_protocol()
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
			case ProtocolType::EXTENDED_GOMOCUP:
				break;
			case ProtocolType::YIXINBOARD:
				break;
		}
	}
	void ProgramManager::process_input_from_user()
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
	void ProgramManager::setup_logging()
	{
		logfile = std::ofstream(argument_parser.getLaunchPath() + "logs" + path_separator + currentDateTime() + ".log");
		Logger::enable();
		Logger::redirectTo(logfile);
	}
	SearchEngine& ProgramManager::getEngine()
	{
		if (search_engine == nullptr)
			search_engine = std::make_unique<SearchEngine>(*engine_settings);
		return *search_engine;
	}
	void ProgramManager::setPosition(const std::vector<Move> &listOfMoves)
	{
		if (listOfMoves.empty())
			sign_to_move = Sign::CROSS;
		else
			sign_to_move = invertSign(listOfMoves.back().sign);

		GameConfig cfg = engine_settings->getGameConfig();
		board = matrix<Sign>(cfg.rows, cfg.cols);
		for (size_t i = 0; i < listOfMoves.size(); i++)
			Board::putMove(board, listOfMoves[i]);
	}

} /* namespace ag */
