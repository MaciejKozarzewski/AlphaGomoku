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

	int number_of_iterations = 0;
	std::string path;
	ArgumentParser ap;
	ap.addArgument("iterations", [&](const std::string &arg)
	{	number_of_iterations = std::stoi(arg);});
	ap.addArgument("path", [&](const std::string &arg)
	{	path = arg;});
	ap.parseArguments(argc, argv);

	if (path.empty())
	{
		std::cout << "Path is empty, exiting" << std::endl;
		return 0;
	}
	if (number_of_iterations <= 0)
	{
		std::cout << "Number of iterations = " << number_of_iterations << " is not positive, exiting" << std::endl;
		return 0;
	}

	if (pathExists(path + "/config.json"))
	{
		TrainingManager tm(path);
		for (int i = 0; i < number_of_iterations; i++)
			tm.runIterationRL();
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
