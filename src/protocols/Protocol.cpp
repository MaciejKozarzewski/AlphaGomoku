/*
 * Protocol.cpp
 *
 *  Created on: Apr 4, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{

	InputListener::InputListener(std::istream &inputStream) :
			input_stream(&inputStream)
	{
	}
	std::string InputListener::getLine()
	{
		return wait_for_line(true);
	}
	std::string InputListener::peekLine()
	{
		return wait_for_line(false);
	}
	void InputListener::consumeLine()
	{
		wait_for_line(true);
	}
	void InputListener::consumeLine(const std::string &line)
	{
		std::string tmp = wait_for_line(true);
		if (tmp != line)
			throw std::logic_error("Expected line '" + line + "', got '" + tmp + "' instead");
	}
	void InputListener::pushLine(const std::string &line)
	{
		std::unique_lock lock(listener_mutex);
		input_queue.push(line);
		listener_cond.notify_all();
	}

	std::string InputListener::wait_for_line(bool consume_result)
	{
		std::unique_lock lock(listener_mutex);
		if (input_queue.empty())
		{
			if (input_stream == nullptr)
			{
				listener_cond.wait(lock, [this] // @suppress("Invalid arguments")
				{	return this->input_queue.empty() == false;});
			}
			else
			{
				lock.unlock();
				std::string line = read_line_from_stream();
				ag::Logger::write("Received : " + line);

				lock.lock();
				input_queue.push(line);
				listener_cond.notify_all();
			}
		}
		std::string result = input_queue.front();
		if (consume_result)
			input_queue.pop();
		return result;
	}
	std::string InputListener::read_line_from_stream()
	{
		std::string result;
		getline(*input_stream, result);
		if (result.back() == '\r') // remove '\r' character from the end, if exists
			result.pop_back();
		return result;
	}

	OutputSender::OutputSender(std::ostream &outputStream) :
			output_stream(outputStream)
	{
	}
	void OutputSender::send(const std::string &msg) const noexcept
	{
		Logger::write("Answered : " + msg);
		std::lock_guard lock(sender_mutex);
		output_stream << msg << std::endl;
	}

	bool Message::isEmpty() const noexcept
	{
		return type == MessageType::EMPTY_MESSAGE;
	}
	bool Message::holdsNoData() const noexcept
	{
		return std::holds_alternative<NoData>(data);
	}
	bool Message::holdsGameConfig() const noexcept
	{
		return std::holds_alternative<GameConfig>(data);
	}
	bool Message::holdsListOfMoves() const noexcept
	{
		return std::holds_alternative<std::vector<Move>>(data);
	}
	bool Message::holdsOption() const noexcept
	{
		return std::holds_alternative<Option>(data);
	}
	bool Message::holdsMove() const noexcept
	{
		return std::holds_alternative<Move>(data);
	}
	bool Message::holdsString() const noexcept
	{
		return std::holds_alternative<std::string>(data);
	}

	MessageType Message::getType() const noexcept
	{
		return type;
	}
	GameConfig Message::getGameConfig() const
	{
		return std::get<GameConfig>(data);
	}
	std::vector<Move> Message::getListOfMoves() const
	{
		return std::get<std::vector<Move>>(data);
	}
	Option Message::getOption() const
	{
		return std::get<Option>(data);
	}
	Move Message::getMove() const
	{
		return std::get<Move>(data);
	}
	std::string Message::getString() const
	{
		return std::get<std::string>(data);
	}

	std::string Message::info() const
	{
		std::string result = "type = ";
		switch (getType())
		{
			case MessageType::EMPTY_MESSAGE:
				result += "EMPTY_MESSAGE";
				break;
			case MessageType::START_PROGRAM:
				result += "START_PROGRAM";
				break;
			case MessageType::SET_OPTION:
				result += "SET_OPTION";
				break;
			case MessageType::SET_POSITION:
				result += "SET_POSITION";
				break;
			case MessageType::START_SEARCH:
				result += "START_SEARCH";
				break;
			case MessageType::STOP_SEARCH:
				result += "STOP_SEARCH";
				break;
			case MessageType::EXIT_PROGRAM:
				result += "EXIT_PROGRAM";
				break;
			case MessageType::ABOUT_ENGINE:
				result += "ABOUT_ENGINE";
				break;
			case MessageType::IS_READY:
				result += "IS_READY";
				break;
			case MessageType::PLAIN_STRING:
				result += "PLAIN_STRING";
				break;
			case MessageType::UNKNOWN_COMMAND:
				result += "UNKNOWN_COMMAND";
				break;
			case MessageType::ERROR:
				result += "ERROR";
				break;
			case MessageType::INFO_MESSAGE:
				result += "INFO_MESSAGE";
				break;
			case MessageType::BEST_MOVE:
				result += "BEST_MOVE";
				break;
		}
		result += ", data";
		if (holdsGameConfig())
			result += "[GameConfig] = " + std::to_string(getGameConfig().rows) + ", " + std::to_string(getGameConfig().cols) + ", "
					+ toString(getGameConfig().rules) + ")";
		if (holdsListOfMoves())
		{
			result += "[ListOfMoves] = {";
			for (size_t i = 0; i < getListOfMoves().size(); i++)
				result += ((i == 0) ? "" : ", ") + getListOfMoves().at(i).toString();
			result += "}";
		}
		if (holdsMove())
			result += "[Move] = " + getMove().toString();
		if (holdsNoData())
			result += "[NoData]";
		if (holdsOption())
			result += "[Option] = " + getOption().name + "=" + getOption().value;
		if (holdsString())
			result += "[String] = " + getString();
		return result;
	}

	void MessageQueue::clear()
	{
		std::lock_guard lock(queue_mutex);
		while (message_queue.empty() == false)
			message_queue.pop();
	}
	bool MessageQueue::isEmpty() const noexcept
	{
		std::lock_guard lock(queue_mutex);
		return message_queue.empty();
	}
	int MessageQueue::length() const noexcept
	{
		std::lock_guard lock(queue_mutex);
		return static_cast<int>(message_queue.size());
	}
	void MessageQueue::push(const Message &msg)
	{
		std::lock_guard lock(queue_mutex);
		message_queue.push(msg);
	}
	Message MessageQueue::pop()
	{
		std::lock_guard lock(queue_mutex);
		Message result = message_queue.front();
		message_queue.pop();
		return result;
	}
	Message MessageQueue::peek() const
	{
		std::lock_guard lock(queue_mutex);
		return message_queue.front();
	}

	std::string toString(ProtocolType pt)
	{
		switch (pt)
		{
			case ProtocolType::GOMOCUP:
				return "GOMOCUP";
			case ProtocolType::YIXINBOARD:
				return "YIXINBOARD";
			case ProtocolType::UGI:
				return "UGI";
			default:
				return "";
		}
	}
	ProtocolType protocolFromString(const std::string &str)
	{
		if (str == "GOMOCUP" or str == "gomocup")
			return ProtocolType::GOMOCUP;
		if (str == "YIXINBOARD" or str == "yixinboard")
			return ProtocolType::YIXINBOARD;
		if (str == "UGI" or str == "ugi")
			return ProtocolType::UGI;
		throw std::invalid_argument(str);
	}

} /* namespace ag */

