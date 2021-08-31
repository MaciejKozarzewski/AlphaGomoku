/*
 * test_argument_parser.cpp
 *
 *  Created on: Aug 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/argument_parser.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestArgument, NoArgumentAction)
	{
		bool flag = false;
		Argument arg("arg");
		arg.action([&]()
		{	flag = true;});

		arg.parse( { });
		EXPECT_TRUE(flag);
	}
	TEST(TestArgument, OneArgumentAction)
	{
		bool flag1 = false;
		bool flag2 = false;
		Argument arg("arg");
		arg.action([&](const std::string &str)
		{	if(str == "flag1")
			flag1 = true;
			if(str == "flag2")
			flag2 = true;
		});

		arg.parse( { "flag" });
		EXPECT_FALSE(flag1);
		EXPECT_FALSE(flag2);

		arg.parse( { "flag1" });
		EXPECT_TRUE(flag1);
		EXPECT_FALSE(flag2);

		arg.parse( { "flag2" });
		EXPECT_TRUE(flag1);
		EXPECT_TRUE(flag2);
	}

	TEST(TestArgumentParser, LaunchPath)
	{
		ArgumentParser ap;

		EXPECT_THROW(ap.parseArguments( { }), ParsingError);

#ifdef _WIN32
		ap.parseArguments( { "this\\is\\sample\\path\\exec" });
		EXPECT_EQ(ap.getLaunchPath(), "this\\is\\sample\\path\\");
#elif __linux__
		ap.parseArguments( { "this/is/sample/path/exec" });
		EXPECT_EQ(ap.getLaunchPath(), "this/is/sample/path/");
#endif
	}
	TEST(TestArgumentParser, PositionalArguments)
	{
		int first_arg = 0;
		float second_arg = 0.0f;
		std::string third_arg = "";

		ArgumentParser ap;
		ap.addArgument("int", [&](const std::string &str)
		{	first_arg = std::stoi(str);});
		ap.addArgument("float", [&](const std::string &str)
		{	second_arg = std::stof(str);});
		ap.addArgument("string", [&](const std::string &str)
		{	third_arg = str;});

		EXPECT_THROW(ap.parseArguments( { "path", "123" }), ParsingError);

		ap.parseArguments( { "path", "123", "12.34f", "text" });
		EXPECT_EQ(first_arg, 123);
		EXPECT_EQ(second_arg, 12.34f);
		EXPECT_EQ(third_arg, "text");
	}
	TEST(TestArgumentParser, OptionalArguments)
	{
		bool flag = false;

		ArgumentParser ap;
		ap.addArgument("--flag").action([&]()
		{	flag = true;});

		EXPECT_NO_THROW(ap.parseArguments( { "path" }));

		ap.parseArguments( { "path", "--flag" });
		EXPECT_TRUE(flag);
	}
	TEST(TestArgumentParser, RequiredOptionalArguments)
	{
		ArgumentParser ap;
		ap.addArgument("--flag").required();
		EXPECT_THROW(ap.parseArguments( { "path" }), ParsingError);
	}
	TEST(TestArgumentParser, UnrecognizedOptionalArguments)
	{
		ArgumentParser ap;
		ap.addArgument("--flag");
		EXPECT_THROW(ap.parseArguments( { "path", "--other" }), ParsingError);
	}
	TEST(TestArgumentParser, ShortNameOptionalArguments)
	{
		ArgumentParser ap;
		ap.addArgument("--flag", "-f");
		EXPECT_THROW(ap.parseArguments( { "path", "--f" }), ParsingError);
		EXPECT_NO_THROW(ap.parseArguments( { "path", "-f" }));
		EXPECT_NO_THROW(ap.parseArguments( { "path", "--flag" }));
	}
	TEST(TestArgumentParser, OptionalArgumentsSeparator)
	{
		int flag = 0;

		ArgumentParser ap;
		ap.addArgument("--flag").action([&](const std::string &arg)
		{	flag = std::stoi(arg);});

		EXPECT_NO_THROW(ap.parseArguments( { "path" }));

		ap.parseArguments( { "path", "--flag", "10" });
		EXPECT_EQ(flag, 10);

		ap.parseArguments( { "path", "--flag=11" });
		EXPECT_EQ(flag, 11);
	}

} /* namespace ag */

