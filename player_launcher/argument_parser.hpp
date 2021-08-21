/*
 * argument_parser.hpp
 *
 *  Created on: Aug 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ARGUMENT_PARSER_HPP_
#define ARGUMENT_PARSER_HPP_

#include <string>
#include <variant>
#include <vector>
#include <exception>
#include <functional>

namespace ag
{
	class ParsingError: public std::logic_error
	{
		public:
			ParsingError(const std::string &msg);
	};

	class Argument
	{
			using no_arg_action = std::function<void()>;
			using one_arg_action = std::function<void(const std::string&)>;
			using multi_arg_action = std::function<void(const std::vector<std::string>&)>;
			std::variant<no_arg_action, one_arg_action, multi_arg_action> m_action;
			std::string m_full_name;
			std::string m_short_name;
			std::string m_help;
			bool m_is_required = false;
			bool m_has_been_parsed = false;
		public:
			Argument(const std::string &fullName, const std::string &shortName);
			Argument(const std::string &fullName);
			Argument& help(const std::string &text);
			Argument& required() noexcept;
			Argument& action(no_arg_action func);
			Argument& action(one_arg_action func);
			Argument& action(multi_arg_action func);

			std::string getShortName() const;
			std::string getFullName() const;
			std::string getHelp() const;
			bool isRequired() const noexcept;
			bool hasBeenParsed() const noexcept;

			void parse(const std::vector<std::string> &args);
	};

	class ArgumentParser
	{
			std::string m_help_message;
			std::string m_local_launch_path;
			std::vector<Argument> m_positional_arguments;
			std::vector<Argument> m_optional_arguments;
		public:
			ArgumentParser(const std::string &helperMessage = "");

			Argument& addArgument(const std::string &shortName, const std::string &fullName);
			Argument& addArgument(const std::string &fullName);
			void parseArguments(const std::vector<std::string> &arguments);
			void parseArguments(int argc, char *argv[]);
			std::string getLocalLaunchPath() const;

			bool hasArgument(const std::string &name) const;
			Argument& getArgument(const std::string &name);

			std::string getHelpMessage(int columns = 80) const;
	};

} /* namespace ag */

#endif /* ARGUMENT_PARSER_HPP_ */
