/*
 * argument_parser.cpp
 *
 *  Created on: Aug 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/argument_parser.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>

namespace
{
	using namespace ag;
	std::pair<std::string, std::string> parse_launch_path(std::string text)
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
		return
		{	text.substr(0, last_slash), text.substr(last_slash)};
	}
	std::string combine(const std::vector<std::string> &args)
	{
		std::string result;
		for (size_t i = 0; i < args.size(); i++)
			result += (i == 0 ? "" : " ") + args[i];
		return result;
	}
	size_t max_name_length(const std::vector<Argument> &args)
	{
		size_t result = 0;
		for (auto iter = args.begin(); iter < args.end(); iter++)
			result = std::max(result, iter->getHelpFormatted().length());
		return result;
	}
	void empty_action()
	{
	}
}

namespace ag
{
	ParsingError::ParsingError(const std::string &msg) :
			std::logic_error(msg)
	{
	}

	Argument::Argument(const std::string &fullName, const std::string &shortName) :
			m_action(empty_action),
			m_full_name(fullName),
			m_short_name(shortName),
			m_type(ArgumentType::OPTIONAL)
	{
	}
	Argument::Argument(const std::string &fullName) :
			m_action(empty_action),
			m_full_name(fullName),
			m_type(ArgumentType::OPTIONAL)
	{
	}
	Argument::Argument(const std::string &fullName, one_arg_action func) :
			m_action(func),
			m_full_name(fullName),
			m_type(ArgumentType::POSITIONAL),
			m_is_required(true)
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
	std::string Argument::getHelpFormatted() const
	{
		std::string result = "  ";
		if (getShortName().empty())
			result += "    ";
		else
			result += getShortName() + ", ";
		result += getFullName();

		if (std::holds_alternative<one_arg_action>(m_action) and m_type == ArgumentType::OPTIONAL)
			result += "=<value>";
		return result;
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
			if (args.size() != 1)
				throw ParsingError("Argument '" + getFullName() + "' accepts exactly one parameter but received '" + combine(args) + "'");
			std::get<one_arg_action>(m_action)(args.front());
		}
		m_has_been_parsed = true;
	}

	ArgumentParser::ArgumentParser(const std::string &helperMessage) :
			m_help_message(helperMessage),
			m_use_for_remaining_arguments([](const std::vector<std::string> &args)
			{	if(args.size() > 0) throw ParsingError("Following arguments were not recognized:" + combine(args));})
	{
	}
	Argument& ArgumentParser::addArgument(const std::string &fullName, const std::string &shortName)
	{
		if (fullName.length() < 3 or fullName[0] != '-' or fullName[1] != '-')
			throw ParsingError("invalid full name");
		if (shortName.length() != 2 or shortName[0] != '-')
			throw ParsingError("invalid short name");
		m_optional_arguments.push_back(Argument(fullName, shortName));
		return m_optional_arguments.back();
	}
	Argument& ArgumentParser::addArgument(const std::string &fullName)
	{
		if (fullName.length() < 3 or fullName[0] != '-' or fullName[1] != '-')
			throw ParsingError("invalid full name");
		m_optional_arguments.push_back(Argument(fullName));
		return m_optional_arguments.back();
	}
	Argument& ArgumentParser::addArgument(const std::string &fullName, one_arg_action func)
	{
		if (fullName.length() >= 2 and (fullName[0] == '-' or fullName[1] == '-'))
			throw ParsingError("invalid full name");
		m_positional_arguments.push_back(Argument(fullName, func));
		return m_positional_arguments.back();
	}
	void ArgumentParser::useForRemainingArguments(multi_arg_action func)
	{
		m_use_for_remaining_arguments = func;
	}

	void ArgumentParser::parseArguments(std::vector<std::string> arguments)
	{
		process_separators(arguments);

		if (arguments.size() == 0)
			throw ParsingError("Parser requires at least one argument - executable launch path");
		auto tmp = parse_launch_path(arguments.front());
		m_launch_path = tmp.first;
		m_executable_name = tmp.second;
		size_t arg_counter = 1;

		if (arguments.size() - 1 < m_positional_arguments.size())
			throw ParsingError(
					"Got only " + std::to_string(arguments.size() - 1) + " positional arguments out of "
							+ std::to_string(m_positional_arguments.size()) + " required");

		// parse positional arguments
		for (size_t i = 0; i < m_positional_arguments.size(); i++, arg_counter++)
			m_positional_arguments.at(i).parse( { arguments.at(arg_counter) });

		// check if all positional arguments have been specified
		for (auto iter = m_positional_arguments.begin(); iter < m_positional_arguments.end(); iter++)
			if (iter->isRequired() and not iter->hasBeenParsed())
				throw ParsingError("Required positional argument '" + iter->getFullName() + "' has not been specified");

		// parse optional arguments
		std::vector<std::string> remaining_args;
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
			{
				remaining_args.push_back(arg_name);
				arg_counter++;
			}
		}

		// check if all required optional arguments have been specified
		for (auto iter = m_optional_arguments.begin(); iter < m_optional_arguments.end(); iter++)
			if (iter->isRequired() and not iter->hasBeenParsed())
				throw ParsingError("Required optional argument '" + iter->getFullName() + "' has not been specified");

		m_use_for_remaining_arguments(remaining_args);
	}
	void ArgumentParser::parseArguments(int argc, char *argv[])
	{
		std::vector<std::string> arguments;
		for (int i = 0; i < argc; i++)
			arguments.push_back(std::string(argv[i]));
		parseArguments(arguments);
	}
	std::string ArgumentParser::getLaunchPath() const
	{
		return m_launch_path;
	}
	std::string ArgumentParser::getExecutablename() const
	{
		return m_executable_name;
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

	std::string ArgumentParser::getHelpMessage(size_t columns) const
	{
		const std::string leading_space = "  ";
		size_t max_full_name_length = std::min(columns / 2, std::max(max_name_length(m_positional_arguments), max_name_length(m_optional_arguments)));

		std::string result = m_help_message + '\n';
		if (not m_positional_arguments.empty())
		{
			result += "Positional arguments\n";
			result += list_arguments(m_positional_arguments, max_full_name_length);
		}

		if (not m_optional_arguments.empty())
		{
			result += "Optional arguments\n";
			result += list_arguments(m_optional_arguments, max_full_name_length);
		}

		return result;
	}
	void ArgumentParser::process_separators(std::vector<std::string> &args) const
	{
		for (size_t i = 0; i < args.size(); i++)
		{
			auto idx = std::find(args[i].begin(), args[i].end(), '=');
			if (idx != args[i].end()) // contains '=' character
			{
				std::string first(args[i].begin(), idx);
				std::string second(idx + 1, args[i].end());
				args[i] = first;
				args.insert(args.begin() + i + 1, second);
			}
		}
	}
	std::string ArgumentParser::list_arguments(const std::vector<Argument> &args, size_t split_pos) const
	{
		std::string result;
		for (auto iter = args.begin(); iter < args.end(); iter++)
		{
			result += iter->getHelpFormatted();
			for (size_t j = iter->getHelpFormatted().length(); j < split_pos; j++)
				result += ' ';
			result += "  " + iter->getHelp() + '\n';
		}
		return result;
	}

}
/* namespace ag */

