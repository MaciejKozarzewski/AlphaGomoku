/*
 * Protocol.cpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{

	InputListener::InputListener():
			input_stream(std::cin)
	{
	}
	InputListener::InputListener(std::istream &inputStream):
			input_stream(inputStream)
	{
		listener_thread = std::thread(run, this);
	}
	InputListener::~InputListener()
	{
		{
			std::lock_guard lock(listener_mutex);
			is_running = false;
		}
		if (listener_thread.joinable())
			listener_thread.join();
	}
	bool InputListener::isRunning() const noexcept
	{
		std::lock_guard lock(listener_mutex);
		return is_running;
	}
	bool InputListener::isEmpty() const noexcept
	{
		std::lock_guard lock(listener_mutex);
		return input_queue.empty();
	}
	int InputListener::length() const noexcept
	{
		std::lock_guard lock(listener_mutex);
		return input_queue.size();
	}

	void InputListener::waitForInput()
	{
		std::unique_lock lock(listener_mutex);
		listener_cond.wait(lock, [this] // @suppress("Invalid arguments")
		{	return this->input_queue.empty() == false; });
	}
	std::string InputListener::getLine()
	{
		std::unique_lock lock(listener_mutex);
		listener_cond.wait(lock, [this] // @suppress("Invalid arguments")
		{	return this->input_queue.empty() == false; });
		std::string result = input_queue.front();
		input_queue.pop();
		return result;
	}
	std::string InputListener::peekLine()
	{
		std::unique_lock lock(listener_mutex);
		listener_cond.wait(lock, [this] // @suppress("Invalid arguments")
		{	return this->input_queue.empty() == false; });
		return input_queue.front();
	}
	void InputListener::pushLine(const std::string &line)
	{
		Logger::write("Received: " + line + '\n');
		std::lock_guard lock(listener_mutex);
		input_queue.push(line);
		listener_cond.notify_all();
	}
	// private
	void InputListener::run(InputListener *arg)
	{
		assert(arg != nullptr);
		while(arg->isRunning())
		{
			std::string line;
			getline(arg->input_stream, line);
			if (line[line.length() - 1] == '\r')
				line = line.substr(0, line.length() - 1);
			if(Protocol::isExitCommand(line))
			{
				std::lock_guard lock(arg->listener_mutex);
				arg->is_running = false;
			}
			arg->pushLine(line);
		}
	}

	OutputSender::OutputSender(std::ostream &outputStream):
			output_stream(outputStream)
	{
	}
	void OutputSender::send(const std::string &msg) const noexcept
	{
		Logger::write("Answered: " + msg + '\n');
		output_stream << msg << std::endl;
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

	bool Protocol::isExitCommand(const std::string &str) noexcept
	{
		return str == "END" or str == "end" or str == "EXIT" or str == "exit";
	}

} /* namespace ag */


