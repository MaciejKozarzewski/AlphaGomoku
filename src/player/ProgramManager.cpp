/*
 * ProgramManager.cpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */
#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/version.hpp>

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/protocols/ExtendedGomocupProtocol.hpp>
#include <alphagomoku/protocols/YixinBoardProtocol.hpp>

#include <alphagomoku/player/controllers/dispatcher.hpp>

#include <iostream>

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
			input_listener(std::cin, false),
			output_sender(std::cout)
	{
		time_manager.startTimer();
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
		if (run_benchmark)
		{
			config = createDefaultConfig();
			setup_paths_in_config();
			benchmark();
			return false;
		}
		bool success = load_config(argument_parser.getLaunchPath() + name_of_config_file);
		if (success)
			setup_paths_in_config();
		return success;
	}
	void ProgramManager::run()
	{
		setup_logging();
		Logger::write("Using config : " + name_of_config_file);

		engine_settings = std::make_unique<EngineSettings>(config);
		setup_protocol();

		time_manager.stopTimer();
		Logger::write("Initialized in " + std::to_string(time_manager.getElapsedTime()));
		while (is_running)
		{
			process_input_from_user();

			while (input_queue.isEmpty() == false)
			{
				const Message input_message = input_queue.pop();
				switch (input_message.getType())
				{
					case MessageType::START_PROGRAM:
					{
						if (search_engine != nullptr)
							search_engine->stopSearch();
						if (game_counter > 0)
							setup_logging();
						game_counter++;
						set_position(std::vector<Move>()); // clear board
						protocol->reset();
						break;
					}
					case MessageType::SET_OPTION:
					{
						const SetOptionOutcome outcome = engine_settings->setOption(input_message.getOption());
						switch (outcome)
						{
							case SetOptionOutcome::SUCCESS:
								break;
							case SetOptionOutcome::SUCCESS_BUT_REALLOCATE_ENGINE:
								if (search_engine != nullptr)
								{
									if (search_engine->isSearchFinished())
										search_engine = nullptr;
									else
										output_queue.push(Message(MessageType::ERROR, "Cannot set this option while the search is running"));
								}
								break;
							case SetOptionOutcome::FAILURE:
								output_queue.push(Message(MessageType::ERROR, std::string("unknown option ") + input_message.getOption().name));
								break;
						}
						break;
					}
					case MessageType::SET_POSITION:
					{
						set_position(input_message.getListOfMoves());
						break;
					}
					case MessageType::START_SEARCH:
					{
						if (search_engine == nullptr)
							setup_engine(); // this may fail and not create engine
						if (search_engine != nullptr)
						{
							if (search_engine->isSearchFinished() == false)
							{
								output_queue.push(Message(MessageType::ERROR, "Search engine is already running"));
								break;
							}
							search_engine->setPosition(board, sign_to_move);
							setup_controller(input_message.getString());
						}
						else
							protocol->reset(); // the protocol does not know about the failure of engine creation, so we must reset its state from here
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
					case MessageType::INFO_MESSAGE:
					{
						if (search_engine != nullptr)
						{
							SearchSummary summary = search_engine->getSummary(input_message.getListOfMoves(), false);
							output_queue.push(Message(MessageType::INFO_MESSAGE, summary));
						}
						break;
					}
					default:
						break;
				}
			}

			if (engine_controller != nullptr)
				engine_controller->control(output_queue);

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
		if (not pathExists(path_to_benchmark))
		{
			output_sender.send(
					"No benchmark file was found, create it by launching " + ProgramInfo::name() + " from command line with parameter '--benchmark'");
			return;
		}

		Json benchmark_results;
		try
		{
			FileLoader fl(path_to_benchmark);
			benchmark_results = fl.getJson();
		} catch (std::exception &e)
		{
			output_sender.send("The benchmark file is invalid for some reason. Try deleting it and creating a new one.");
			return;
		}

		if (static_cast<std::string>(benchmark_results["version"]) != ProgramInfo::version())
		{
			output_sender.send(
					"Existing benchmark file is for different version, create a new one by launching " + ProgramInfo::name()
							+ " from command line with parameter '--benchmark'");
			return;
		}
		Json cfg = createConfig(benchmark_results);
		output_sender.send("Created new configuration file.");
		output_sender.send(cfg.dump(2));

		FileSaver fs(argument_parser.getLaunchPath() + "config.json");
		fs.save(cfg, SerializedObject(), 2, false);
	}
	bool ProgramManager::load_config(const std::string &path)
	{
		if (pathExists(path))
		{
			try
			{
				FileLoader fl(path);
				config = fl.getJson();
				return true;
			} catch (std::exception &e)
			{
				output_sender.send("The configuration file is invalid for some reason. Try deleting it and creating a new one.");
			}
		}
		else
		{
			output_sender.send("Could not load configuration file.");
			output_sender.send("You can generate new one by launching " + ProgramInfo::name() + " from command line with parameter '--configure'");
			output_sender.send(
					"If you are sure that configuration file exists, it means that the launch path was not parsed correctly (probably because of some special characters in it).");
		}
		return false;
	}
	void ProgramManager::setup_paths_in_config()
	{
		std::string launch_path = argument_parser.getLaunchPath();
		config["swap2_openings_file"] = launch_path + config["swap2_openings_file"].getString();
		launch_path += "networks" + path_separator;

		const std::array<std::string, 5> tmp = { "freestyle", "standard", "renju", "caro5", "caro6" };
		for (auto iter = tmp.begin(); iter < tmp.end(); iter++)
		{
			config["conv_networks"][*iter] = launch_path + config["conv_networks"][*iter].getString();
			config["nnue_networks"][*iter] = launch_path + config["nnue_networks"][*iter].getString();
		}
	}

	void ProgramManager::setup_protocol()
	{
		if (not config.hasKey("protocol"))
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
				protocol = std::make_unique<ExtendedGomocupProtocol>(input_queue, output_queue);
				break;
			case ProtocolType::YIXINBOARD:
				protocol = std::make_unique<YixinBoardProtocol>(input_queue, output_queue);
				break;
			default:
				throw std::logic_error("Unknown protocol type '" + config["protocol"].getString() + "'");
		}
		assert(protocol != nullptr);
		Logger::write("Using " + toString(protocol->getType()) + " protocol");
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
			{
				try
				{
					protocol->processInput(input_listener);
				}
				catch(std::exception &e)
				{
					output_queue.push(Message(MessageType::ERROR, e.what()));
					ag::Logger::write(e.what());
				}
			});
		}
	}
	void ProgramManager::setup_logging()
	{
		if (static_cast<bool>(config["use_logging"]))
		{
			if (not pathExists(argument_parser.getLaunchPath() + "logs"))
				createDirectory(argument_parser.getLaunchPath() + "logs");
			logfile = std::ofstream(argument_parser.getLaunchPath() + "logs" + path_separator + currentDateTime() + ".log");
			Logger::enable();
			Logger::redirectTo(logfile);
		}
	}
	void ProgramManager::setup_engine()
	{
		assert(search_engine == nullptr);
		if (is_game_config_correct())
		{
			time_manager.startTimer(); // the time of initialization of the search engine should be included in the time of the first move
			search_engine = std::make_unique<SearchEngine>(*engine_settings);
			time_manager.stopTimer();
			Logger::write("Set up engine in " + std::to_string(time_manager.getElapsedTime()));
		}
		else
		{
			GameConfig cfg = engine_settings->getGameConfig();
			Message msg(MessageType::ERROR,
					toString(cfg.rules) + " rule is not supported on " + std::to_string(cfg.rows) + "x" + std::to_string(cfg.cols) + " board");
			output_queue.push(msg);
		}
	}
	void ProgramManager::set_position(const std::vector<Move> &listOfMoves)
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
	void ProgramManager::setup_controller(const std::string &type)
	{
		if (type.empty())
			return; // using the same controller as previously
		std::vector<std::string> tmp = split(type, ' ');
		assert(tmp.size() >= 1);
		assert(engine_settings != nullptr);
		assert(search_engine != nullptr);

		if (tmp[0] == "clearhash") // it just pretends to be a controller but does not start any search
			return;

		engine_controller = createController(tmp[0], *engine_settings, time_manager, *search_engine);
		if (engine_controller != nullptr)
		{
			std::string args = type;
			args.erase(args.begin(), args.begin() + tmp[0].size());
			engine_controller->setup(args);
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Unsupported controller type '" + type + "'"));
	}
	bool ProgramManager::is_game_config_correct() const
	{
		GameConfig cfg = engine_settings->getGameConfig();
		switch (cfg.rules)
		{
			case GameRules::FREESTYLE:
				return cfg.rows == 20 and cfg.cols == 20;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO5:
			case GameRules::CARO6:
				return cfg.rows == 15 and cfg.cols == 15;
			default:
				return false;
		}
	}

} /* namespace ag */

