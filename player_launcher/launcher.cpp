/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/version.hpp>

#include "engine_manager.hpp"

int main(int argc, char *argv[])
{
	ag::EngineManager engine_manager;
	try
	{
		engine_manager.processArguments(argc, argv);
		engine_manager.run();
		ag::Logger::write(ag::ProgramInfo::name() + " successfully exits");
		return 0;
	} catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		ag::Logger::write(e.what());
		return -1;
	}
}

