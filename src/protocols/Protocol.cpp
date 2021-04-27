/*
 * Protocol.cpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace
{
	std::string get_line(std::istream &stream)
	{
		std::string result;
		getline(stream, result);
		if (result[result.length() - 1] == '\r')
			result = result.substr(0, result.length() - 1);

		ag::Logger::write("Received : " + result);
		return result;
	}
}

namespace ag
{

	InputListener::InputListener(std::istream &inputStream) :
			input_stream(&inputStream)
	{
	}
	std::string InputListener::getLine()
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
				input_queue.push(get_line(*input_stream));
				listener_cond.notify_all();
			}
		}
		std::string result = input_queue.front();
		input_queue.pop();
		return result;
	}
	std::string InputListener::peekLine()
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
				input_queue.push(get_line(*input_stream));
				listener_cond.notify_all();
			}
		}
		std::string result = input_queue.front();
		return result;
	}
	void InputListener::pushLine(const std::string &line)
	{
		std::lock_guard lock(listener_mutex);
		input_queue.push(line);
		listener_cond.notify_all();
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
		if (str == "UGI" or str == "ugi")
			return ProtocolType::UGI;
		throw std::invalid_argument(str);
	}

} /* namespace ag */

