/*
 * argument_parser.cpp
 *
 *  Created on: Aug 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include "argument_parser.hpp"
#include <alphagomoku/utils/misc.hpp>

namespace
{
	std::string get_local_launch_path(std::string text)
	{
#ifdef _WIN32
		const std::string path_separator = "\\";
#elif __linux__
		const std::string path_separator = "/";
#endif
		size_t last_slash = text.length();
		for (size_t i = 0; i < text.length(); i++)
			if (text[i] == '\\' or text[i] == '/')
			{
				text[i] = path_separator[0];
				last_slash = i + 1;
			}
		return text.substr(0, last_slash);
	}
	std::string combine(const std::vector<std::string> &args)
	{
		std::string result;
		for (size_t i = 0; i < args.size(); i++)
			result += (i == 0 ? "" : " ") + args[i];
		return result;
	}
}

namespace ag
{
	ParsingError::ParsingError(const std::string &msg) :
			std::logic_error(msg)
	{
	}

	Argument::Argument(const std::string &fullName, const std::string &shortName) :
			m_full_name(fullName),
			m_short_name(shortName)
	{
	}
	Argument::Argument(const std::string &fullName) :
			m_full_name(fullName)
	{
	}
	Argument& Argument::help(const std::string &text)
	{
		m_help = text;
		return *this;
	}
	Argument& Argument::required() noexcept
	{
		m_is_required = true;
		return *this;
	}
	Argument& Argument::action(no_arg_action func)
	{
		m_action = std::move(func);
		return *this;
	}
	Argument& Argument::action(one_arg_action func)
	{
		m_action = std::move(func);
		return *this;
	}
	Argument& Argument::action(multi_arg_action func)
	{
		m_action = std::move(func);
		return *this;
	}

	std::string Argument::getShortName() const
	{
		return m_short_name;
	}
	std::string Argument::getFullName() const
	{
		return m_full_name;
	}
	std::string Argument::getHelp() const
	{
		return m_help;
	}
	bool Argument::isRequired() const noexcept
	{
		return m_is_required;
	}
	bool Argument::hasBeenParsed() const noexcept
	{
		return m_has_been_parsed;
	}
	void Argument::parse(const std::vector<std::string> &args)
	{
		if (std::holds_alternative<no_arg_action>(m_action))
		{
			if (args.size() != 0)
				throw ParsingError("Argument '" + getFullName() + "' does not accept any parameters but received '" + combine(args) + "'");
			std::get<no_arg_action>(m_action)();
		}
		else
		{
			if (std::holds_alternative<one_arg_action>(m_action))
			{
				if (args.size() != 1)
					throw ParsingError("Argument '" + getFullName() + "' accepts exactly one parameter but received '" + combine(args) + "'");
				std::get<one_arg_action>(m_action)(args.front());
			}
			else // multi_arg_action
			{
				std::get<multi_arg_action>(m_action)(args);
			}
		}
		m_has_been_parsed = true;
	}

	ArgumentParser::ArgumentParser(const std::string &helperMessage) :
			m_help_message(helperMessage)
	{
	}
	Argument& ArgumentParser::addArgument(const std::string &shortName, const std::string &fullName)
	{
		m_optional_arguments.push_back(Argument(shortName, fullName));
		return m_optional_arguments.back();
	}
	Argument& ArgumentParser::addArgument(const std::string &fullName)
	{
		if (startsWith(fullName, "--") or startsWith(fullName, "-"))
		{
			m_optional_arguments.push_back(Argument(fullName));
			m_optional_arguments.back().required();
			return m_optional_arguments.back();
		}
		else
		{
			m_positional_arguments.push_back(Argument(fullName));
			return m_positional_arguments.back();
		}
	}
	void ArgumentParser::parseArguments(const std::vector<std::string> &arguments)
	{
		size_t arg_counter = 0;
		if (arguments.size() == 0)
			throw ParsingError("Parser requires at least one argument - executable launch path");
		m_local_launch_path = get_local_launch_path(arguments[arg_counter]);
		arg_counter++;

		if (arguments.size() - 1 < m_positional_arguments.size())
			throw ParsingError(
					"Got only " + std::to_string(arguments.size() - 1) + " positional arguments out of " + std::to_string(m_positional_arguments.size()) + " required");

		// parse positional arguments
		for (size_t i = 0; i < m_positional_arguments.size(); i++, arg_counter++)
			m_positional_arguments.at(i).parse( { arguments.at(arg_counter) });

		for (auto iter = m_positional_arguments.begin(); iter < m_positional_arguments.end(); iter++)
			if (iter->isRequired() and not iter->hasBeenParsed())
				throw ParsingError("Required positional argument '" + iter->getFullName() + "' has not been specified");

		// parse optional arguments
		while (arg_counter < arguments.size())
		{
			std::string arg_name = arguments.at(arg_counter);
			if (hasArgument(arg_name))
			{
				arg_counter++; // move past the current string
				std::vector<std::string> tmp_args;
				for (size_t i = arg_counter; i < arguments.size(); i++)
					if (hasArgument(arguments.at(i)))
						break; // tested string is a next argument
					else
						tmp_args.push_back(arguments.at(i));

				getArgument(arg_name).parse(tmp_args);
				arg_counter += tmp_args.size();
			}
			else
				throw ParsingError("Unrecognized argument '" + arg_name + "'");
		}

		for (auto iter = m_optional_arguments.begin(); iter < m_optional_arguments.end(); iter++)
			if (iter->isRequired() and not iter->hasBeenParsed())
				throw ParsingError("Required optional argument '" + iter->getFullName() + "' has not been specified");
	}
	void ArgumentParser::parseArguments(int argc, char *argv[])
	{
		std::vector<std::string> arguments;
		for (int i = 0; i < argc; i++)
			arguments.push_back(std::string(argv[i]));
		parseArguments(arguments);
	}
	std::string ArgumentParser::getLocalLaunchPath() const
	{
		return m_local_launch_path;
	}
	bool ArgumentParser::hasArgument(const std::string &name) const
	{
		return std::any_of(m_optional_arguments.begin(), m_optional_arguments.end(), [&](const Argument &arg)
		{	return arg.getFullName() == name or arg.getShortName() == name;});
	}
	Argument& ArgumentParser::getArgument(const std::string &name)
	{
		auto result = std::find_if(m_optional_arguments.begin(), m_optional_arguments.end(), [&](const Argument &arg)
		{	return arg.getFullName() == name or arg.getShortName() == name;});
		if (result == m_optional_arguments.end())
			throw ParsingError("Could not find argument '" + name + "'");
		return *result;
	}

	std::string ArgumentParser::getHelpMessage(int columns) const
	{
		return "";
	}

}
/* namespace ag */

