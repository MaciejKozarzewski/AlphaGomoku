/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/version.hpp>

#include <stdexcept>
#include <iostream>
#include <string>

using namespace ag;

int main(int argc, char *argv[])
{
	ml::Device::flushDenormalsToZero(true);
	ag::ProgramManager player_manager;
	try
	{
		const bool can_continue = player_manager.processArguments(argc, argv);
		if (can_continue)
		{
			player_manager.run();
			ag::Logger::write(ag::ProgramInfo::name() + " successfully exits");
		}
		return 0;
	} catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		ag::Logger::write(e.what());
	}
	return -1;
}

