/*
 * Protocol.hpp
 *
 *  Created on: 4 kwi 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <variant>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <iostream>

namespace ag
{
	class InputListener
	{
		private:
			std::istream *input_stream = nullptr;
			std::queue<std::string> input_queue;
			mutable std::mutex listener_mutex;
			std::condition_variable listener_cond;

		public:
			InputListener() = default;
			InputListener(std::istream &inputStream);
			InputListener(const InputListener &other) = delete;
			InputListener(InputListener &&other) = default;
			InputListener& operator=(const InputListener &other) = delete;
			InputListener& operator=(InputListener &&other) = default;
			~InputListener() = default;

			std::string getLine();
			std::string peekLine();
			void pushLine(const std::string &line);
	};

	class OutputSender
	{
		private:
			std::ostream &output_stream;
			mutable std::mutex sender_mutex;
		public:
			OutputSender(std::ostream &outputStream);
			void send(const std::string &msg) const noexcept;
	};

	struct NoData
	{
	};
	struct Option
	{
			std::string name;
			std::string value;
	};

	/**
	 * Enumeration used to tell what to do with data received from user or engine.
	 */
	enum class MessageType
	{
		CHANGE_PROTOCOL, // send to switch between different protocols
		START_PROGRAM, // send to initialize program with specific board size
		SET_OPTION, // send with option name and its value (unparsed)
		SET_POSITION, // used to set specific position
		START_SEARCH, // request to start searching with specific option what to do after the search ends
		STOP_SEARCH, // request to immediately stop search
		MAKE_MOVE, // used to send move made either by user or engine
		EXIT_PROGRAM, // send to exit the program
		EMPTY_MESSAGE, // specifies empty message with no data
		PLAIN_STRING, // message containing string with no specific structure
		UNKNOWN_COMMAND, // used as a response after unknown message from user
		ERROR, // used as a response after an error
		INFO_MESSAGE, // used to send some information string from engine
		ABOUT_ENGINE // request to send some information about the engine
	};
	class Message
	{
		private:
			MessageType type;
			std::variant<NoData, GameConfig, std::vector<Move>, Option, Move, std::string> data;
		public:
			Message(MessageType mt = MessageType::EMPTY_MESSAGE) :
					type(mt)
			{
			}
			template<typename T>
			Message(MessageType mt, const T &arg) :
					type(mt),
					data(arg)
			{
			}
			bool isEmpty() const noexcept;
			bool holdsNoData() const noexcept;
			bool holdsGameConfig() const noexcept;
			bool holdsListOfMoves() const noexcept;
			bool holdsOption() const noexcept;
			bool holdsMove() const noexcept;
			bool holdsString() const noexcept;

			MessageType getType() const noexcept;
			GameConfig getGameConfig() const;
			std::vector<Move> getListOfMoves() const;
			Option getOption() const;
			Move getMove() const;
			std::string getString() const;

			std::string info() const;
	};

	class MessageQueue
	{
		private:
			std::queue<Message> message_queue;
			mutable std::mutex queue_mutex;
		public:
			void clear();
			int length() const noexcept;
			bool isEmpty() const noexcept;
			void push(const Message &msg);
			Message pop();
			Message peek() const;
	};

	enum class ProtocolType
	{
		GOMOCUP, // protocol used in Gomocup tournament
		UGI // protocol derived from Universal Chess Interface, to be specified and implemented later
	};
	std::string toString(ProtocolType pt);
	ProtocolType protocolFromString(const std::string &str);
	class Protocol
	{
		protected:
			MessageQueue &input_queue;
			MessageQueue &output_queue;
		public:
			Protocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
					input_queue(queueIN),
					output_queue(queueOUT)
			{
			}
			Protocol(const Protocol &other) = delete;
			Protocol(Protocol &&other) = delete;
			Protocol& operator=(const Protocol &other) = delete;
			Protocol& operator=(Protocol &&other) = delete;
			virtual ~Protocol() = default;

			virtual ProtocolType getType() const noexcept = 0;
			virtual void processInput(InputListener &listener) = 0;
			virtual void processOutput(OutputSender &sender) = 0;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_ */
