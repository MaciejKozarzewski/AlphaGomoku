/*
 * Protocol.hpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_

#include <alphagomoku/mcts/Move.hpp>

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
			std::istream &input_stream;
			std::queue<std::string> input_queue;
			mutable std::mutex listener_mutex;
			std::condition_variable listener_cond;
			std::thread listener_thread;
			bool is_running = true;

		public:
			/**
			 * Default constructor that does not start separate thread. Can only receive input via 'pushLine' method.
			 */
			InputListener();
			InputListener(std::istream &inputStream);
			InputListener(const InputListener &other) = delete;
			InputListener(InputListener &&other) = default;
			InputListener& operator=(const InputListener &other) = delete;
			InputListener& operator=(InputListener &&other) = default;
			~InputListener();

			bool isRunning() const noexcept;
			bool isEmpty() const noexcept;
			int length() const noexcept;

			void waitForInput();
			std::string getLine();
			void pushLine(const std::string &line);
		private:
			static void run(InputListener *arg);
	};

	class OutputSender
	{
			std::ostream &output_stream;
		public:
			OutputSender(std::ostream &outputStream);
			void send(const std::string &msg) const noexcept;
	};

	struct NoData
	{
	};
	struct BoardSize
	{
			int rows = 0;
			int cols = 0;
	};
	struct Position
	{
			std::vector<Move> moves;
			Sign sign_to_move = Sign::CROSS;
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
		MAKE_MOVE, // used to send move made either by user or engine
		ABOUT_ENGINE, // request to send some information about the engine
		STOP_SEARCH, // send to immediately stop search
		EXIT_PROGRAM, // send to exit the program
		EMPTY_MESSAGE, // specifies empty message with no data
		PLAIN_STRING, // message containing string with no specific structure
		UNKNOWN_MESSAGE, // used as a response after unknown message from user
		ERROR // used as a response after an error
	};
	class Message
	{
		private:
			MessageType type;
			std::variant<NoData, BoardSize, Position, Option, Move, std::string> data;
		public:
			Message(MessageType mt = MessageType::EMPTY_MESSAGE);
			bool isEmpty() const noexcept;
			MessageType getType() const noexcept;
			BoardSize getBoardSize() const;
			Position getPosition() const;
			Option getOption() const;
			Move getMove() const;
			std::string getString() const;
	};

	enum class ProtocolType
	{
		GOMOCUP, // protocol used in Gomocup tournament
		UGI // protocol derived from Universal Chess Interface, to be specified and implemented later
	};
	class Protocol
	{
		public:
			Protocol() = default;
			Protocol(const Protocol &other) = delete;
			Protocol(Protocol &&other) = delete;
			Protocol& operator=(const Protocol &other) = delete;
			Protocol& operator=(Protocol &&other) = delete;
			virtual ~Protocol() = default;

			virtual ProtocolType getType() const noexcept = 0;
			virtual Message processInput(InputListener &listener) const = 0;
			virtual bool processOutput(const Message &msg, OutputSender &sender) const  = 0;

			static bool isExitCommand(const std::string &str) noexcept;
	};

} /* namespace ag */



#endif /* ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_ */
