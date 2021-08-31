/*
 * engine_manager.cpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */
#include <alphagomoku/version.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>

#include "engine_manager.hpp"

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
		argument_parser.parseArguments(argc, argv);
		if (display_help)
			print_help_and_exit();
		if (display_version)
			print_version_and_exit();
		load_config(argument_parser.getLaunchPath() + name_of_config);
		if (run_configuration)
			run_configuration_and_exit();
		if (run_benchmark)
			run_benchmark_and_exit();
	}

	void EngineManager::create_arguments()
	{
		argument_parser.addArgument("--help", "-h").help("display this help text and exit").action([this]()
		{	this->display_help = true;});
		argument_parser.addArgument("--version", "-v").help("display version information and exit").action([this]()
		{	this->display_version = true;});
		argument_parser.addArgument("--load-config").help("loads config named <value> with path relative to the executable. "
				"If not specified, program will load file \"config.json\".").action([this](const std::string &arg)
		{	this ->name_of_config = arg;});
		argument_parser.addArgument("--configure").help(
				"runs configuration mode and exits. Resulting configuration file is saved as <value> in path relative to the executable. "
						"If such file already exists, it will be edited.").action([this](const std::string &arg)
		{	this->run_configuration = true;
			this->name_of_config = arg;});
		argument_parser.addArgument("--benchmark", "-b").help("tests speed of all available hardware, saves to file \"benchmark.json\" and exits").action(
				[this]()
				{	this->run_benchmark = true;});
	}

	void EngineManager::print_help_and_exit() const
	{
		std::string result = argument_parser.getHelpMessage();
		output_sender.send(result);
		exit(0);
	}
	void EngineManager::print_version_and_exit() const
	{
		std::string result = argument_parser.getExecutablename() + " (" + ProgramInfo::name() + ") " + ProgramInfo::version() + '\n';
		result += ProgramInfo::copyright() + '\n';
		result += ProgramInfo::build() + '\n';
		result += ProgramInfo::license() + '\n';
		result += "For more information visit " + ProgramInfo::website() + '\n';
		output_sender.send(result);
		exit(0);
	}
	void EngineManager::run_benchmark_and_exit() const
	{
		Json benchmark_result = ag::run_benchmark(config["networks"]["standard"], output_sender);

		FileSaver fs(argument_parser.getLaunchPath() + "benchmark.json");
		fs.save(benchmark_result, SerializedObject(), 4, false);
		exit(0);
	}
	void EngineManager::run_configuration_and_exit()
	{

	}
	void EngineManager::load_config(const std::string &path)
	{
		if (std::filesystem::exists(path))
		{
			try
			{
				FileLoader fl(path);
				config = fl.getJson();
				std::string launch_path = argument_parser.getLaunchPath();
				config["swap2_openings_file"] = launch_path + static_cast<std::string>(config["swap2_openings_file"]);
				launch_path += "networks" + path_separator;
				config["networks"]["freestyle"] = launch_path + static_cast<std::string>(config["networks"]["freestyle"]);
				config["networks"]["standard"] = launch_path + static_cast<std::string>(config["networks"]["standard"]);
			} catch (std::exception &e)
			{
				output_sender.send("The configuration file is invalid for some reason. Try deleting it and creating a new one.");
				exit(0);
			}
		}
		else
		{
			output_sender.send("Could not load configuration file.");
			output_sender.send("You can generate new one by launching " + ProgramInfo::name() + " from command line with parameter '--configure'");
			output_sender.send(
					"If you are sure that configuration file exists, it means that the launch path might not have been parsed correctly due to some special characters in it.");
			exit(0);
		}
	}
} /* namespace ag */

