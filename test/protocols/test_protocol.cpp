/*
 * test_protocol.cpp
 *
 *  Created on: Apr 5, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/Protocol.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestInputListener, from_stringstream)
	{
		std::istringstream iss("command\nexit\n");
		InputListener listener(iss);

		EXPECT_EQ(listener.getLine(), "command");
		EXPECT_EQ(listener.getLine(), "exit");
	}
	TEST(TestInputListener, push_line)
	{
		InputListener listener;
		listener.pushLine("command");
		EXPECT_EQ(listener.getLine(), "command");
	}
	TEST(TestInputListener, async_push_line)
	{
		InputListener listener;
		std::string line;
		// first thread starts and waits for input
		std::thread thread1([&]
		{	line = listener.getLine();});

		// second thread pushes line to listener to be picked by first thread
		std::thread thread2([&]
		{	std::this_thread::sleep_for(std::chrono::seconds(1));
			listener.pushLine("command");});

		thread1.join();
		thread2.join();
		EXPECT_EQ(line, "command");
	}

} /* namespace ag */

