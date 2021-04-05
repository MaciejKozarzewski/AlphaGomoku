/*
 * test_protocol.cpp
 *
 *  Created on: 5 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/Protocol.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestInputListener, from_stringstream)
	{
		std::istringstream iss("command\nexit\n");
		InputListener listener(iss);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		EXPECT_FALSE(listener.isEmpty());
		EXPECT_EQ(listener.length(), 2);

		std::string first_line = listener.getLine();
		std::string second_line = listener.getLine();

		EXPECT_EQ(first_line, "command");
		EXPECT_EQ(second_line, "exit");
		EXPECT_TRUE(listener.isEmpty());
	}
	TEST(TestInputListener, push_line)
	{
		InputListener listener;
		listener.pushLine("command");
		EXPECT_FALSE(listener.isEmpty());
		EXPECT_EQ(listener.length(), 1);

		EXPECT_EQ(listener.getLine(), "command");
		EXPECT_TRUE(listener.isEmpty());
	}
	TEST(TestInputListener, async_push_line)
	{
		InputListener listener;
		std::thread thread([&]
		{ std::this_thread::sleep_for(std::chrono::seconds(1));
		  listener.pushLine("command");	});

		EXPECT_TRUE(listener.isEmpty());
		std::string line = listener.getLine();
		EXPECT_EQ(line, "command");
		thread.join();
	}
	TEST(TestInputListener, wait_for_input)
	{
		InputListener listener;
		std::thread thread([&]
		{ std::this_thread::sleep_for(std::chrono::seconds(1));
		  listener.pushLine("command");
		  listener.pushLine("command");});

		EXPECT_TRUE(listener.isEmpty());
		listener.waitForInput();
		listener.waitForInput();
		EXPECT_EQ(listener.length(), 2);
		thread.join();
	}

} /* namespace ag */



