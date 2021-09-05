/*
 * ArgumentParser.hpp
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

	using no_arg_action = std::function<void()>;
	using one_arg_action = std::function<void(const std::string&)>;
	using multi_arg_action = std::function<void(const std::vector<std::string>&)>;

	class Argument
	{
			enum class ArgumentType
			{
				POSITIONAL, /**< such argument must be specified at correct position */
				OPTIONAL /**< such argument can be specified at any position (or not at all) */
			};
			std::variant<no_arg_action, one_arg_action> m_action; /**< action to perform when argument is parsed */
			std::string m_full_name; /**< full name of a argument */
			std::string m_short_name; /**< shortened name of a argument */
			std::string m_help; /**< help message for a argument */
			ArgumentType m_type; /**< type of a argument */
			bool m_is_required = false; /**< flag indicating whether an argument must be specified, always true for positional ones*/
			bool m_has_been_parsed = false;
		public:
			/**
			 * @brief Creates optional argument.
			 */
			Argument(const std::string &fullName, const std::string &shortName);
			/**
			 * @brief Creates optional argument, but without short name.
			 */
			Argument(const std::string &fullName);
			/**
			 * @brief Creates positional argument.
			 */
			Argument(const std::string &fullName, one_arg_action func);
			Argument& help(const std::string &text);
			Argument& required() noexcept;
			Argument& action(no_arg_action func);
			Argument& action(one_arg_action func);

			std::string getShortName() const;
			std::string getFullName() const;
			std::string getHelp() const;
			std::string getHelpFormatted() const;
			bool isRequired() const noexcept;
			bool hasBeenParsed() const noexcept;

			void parse(const std::vector<std::string> &args);
	};

	class ArgumentParser
	{
			std::string m_help_message;
			std::string m_launch_path;
			std::string m_executable_name;
			std::vector<Argument> m_positional_arguments;
			std::vector<Argument> m_optional_arguments;
			multi_arg_action m_use_for_remaining_arguments;
		public:
			ArgumentParser(const std::string &helperMessage = "");

			/**
			 * @brief Adds new optional argument.
			 */
			Argument& addArgument(const std::string &fullName, const std::string &shortName);
			/**
			 * @brief Adds new optional argument, but without short name.
			 */
			Argument& addArgument(const std::string &fullName);
			/**
			 * @brief Adds new positional argument with a specific action to perform on the passed value.
			 */
			Argument& addArgument(const std::string &fullName, one_arg_action func);
			/**
			 * @brief Specifies what to do with the unparsed arguments.
			 */
			void useForRemainingArguments(multi_arg_action func);

			void parseArguments(std::vector<std::string> arguments);
			void parseArguments(int argc, char *argv[]);
			std::string getLaunchPath() const;
			std::string getExecutablename() const;

			bool hasArgument(const std::string &name) const;
			Argument& getArgument(const std::string &name);

			std::string getHelpMessage(size_t columns = 80) const;
		private:
			void process_separators(std::vector<std::string> &args) const;
			std::string list_arguments(const std::vector<Argument> &args, size_t split_pos) const;
	};

} /* namespace ag */

#endif /* ARGUMENT_PARSER_HPP_ */
