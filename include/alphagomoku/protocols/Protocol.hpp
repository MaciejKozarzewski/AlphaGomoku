/*
 * Protocol.hpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_

#include <alphagomoku/mcts/Move.hpp>
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
			std::istream &input_stream;
			std::queue<std::string> input_queue;
			mutable std::mutex listener_mutex;
			std::condition_variable listener_cond;
			std::thread listener_thread;
			bool is_running = true;

		public:
			/**
			 * Default constructor does not start separate thread. Can only receive input via 'pushLine' method.
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
			std::string peekLine();
			void pushLine(const std::string &line);
		private:
			static void run(InputListener *arg);
	};

	class OutputSender
	{
		private:
			std::ostream &output_stream;
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
		MAKE_MOVE, // used to send move made either by user or engine
		ABOUT_ENGINE, // request to send some information about the engine
		STOP_SEARCH, // request to immediately stop search
		EXIT_PROGRAM, // send to exit the program
		EMPTY_MESSAGE, // specifies empty message with no data
		PLAIN_STRING, // message containing string with no specific structure
		UNKNOWN_MESSAGE, // used as a response after unknown message from user
		ERROR, // used as a response after an error
		OPENING_PRO,
		OPENING_LONG_PRO,
		OPENING_SWAP,
		OPENING_SWAP2
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
			virtual Message processInput(InputListener &listener) = 0;
			virtual bool processOutput(const Message &msg, OutputSender &sender) = 0;

			static bool isExitCommand(const std::string &str) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_ */