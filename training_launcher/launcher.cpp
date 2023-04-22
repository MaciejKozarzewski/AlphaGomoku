/*
 * launcher.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/utils/ArgumentParser.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include "minml/core/Device.hpp"
#include "minml/utils/json.hpp"
#include "minml/utils/serialization.hpp"

#include <iostream>
#include <string>

using namespace ag;

int main(int argc, char *argv[])
{
	std::cout << ml::Device::hardwareInfo() << '\n';
	setupSignalHandler(SignalType::INT, SignalHandlerMode::CUSTOM);

	std::string mode;
	int number_of_iterations = 0;
	std::string path;
	std::string data_path;
	ArgumentParser ap;
	ap.addArgument("mode", [&](const std::string &arg)
	{	mode = arg;});
	ap.addArgument("iterations", [&](const std::string &arg)
	{	number_of_iterations = std::stoi(arg);});
	ap.addArgument("path", [&](const std::string &arg)
	{	path = arg;});
	ap.addArgument("--data", [&](const std::string &arg)
	{	data_path = arg;});
	ap.parseArguments(argc, argv);

	if (mode != "rl" and mode != "sl")
	{
		std::cout << "Mode can be either 'rl' (Reinforcement Learning) or 'sl' (Supervised Learning) but got '" << mode << "', exiting" << std::endl;
		return -1;
	}
	if (number_of_iterations <= 0)
	{
		std::cout << "Number of iterations = " << number_of_iterations << " is not positive, exiting" << std::endl;
		return -1;
	}
	if (path.empty())
	{
		std::cout << "Path is empty, exiting" << std::endl;
		return -1;
	}

	if (pathExists(path + "/config.json"))
	{
		if (mode == "rl")
		{
			TrainingManager tm(path);
			for (int i = 0; i < number_of_iterations; i++)
				tm.runIterationRL();
		}
		if (mode == "sl")
		{
			TrainingManager tm(path, data_path);
			for (int i = 0; i < number_of_iterations; i++)
				tm.runIterationSL();
		}
	}
	else
	{
		if (not pathExists(path))
		{
			createDirectory(path);
			std::cout << "Created working directory" << std::endl;
		}
		MasterLearningConfig cfg;
		FileSaver fs(path + "/config.json");
		fs.save(cfg.toJson(), SerializedObject(), 2, false);
		std::cout << "Created default configuration file, exiting." << std::endl;
	}
	return 0;
}
