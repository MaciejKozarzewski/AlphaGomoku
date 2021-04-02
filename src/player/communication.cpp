/*
 * communication.cpp
 *
 *  Created on: Apr 3, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/communication.hpp>

namespace ag
{

	InputListener::InputListener()
	{
	}
	InputListener::~InputListener()
	{
		{
			std::lock_guard lock(listener_mutex);
			is_running = false;
		}
		listener_thread.join();
	}
	bool InputListener::isEmpty() const noexcept
	{
	}

	std::string InputListener::getLine()
	{
	}
	void InputListener::push(const std::string &line)
	{
	}

	void InputListener::run()
	{
//		std::string line;
//		getline(std::cin, line);
//
//		if (line[line.length() - 1] == '\r')
//			return line.substr(0, line.length() - 1);
//		else
//			return line;
	}

} /* namespace ag */

