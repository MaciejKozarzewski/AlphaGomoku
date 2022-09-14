/*
 * Protocol.hpp
 *
 *  Created on: Apr 4, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
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
			bool case_sensitive = false;

		public:
			InputListener() = default;
			InputListener(std::istream &inputStream, bool caseSensitive = false);
			InputListener(const InputListener &other) = delete;
			InputListener(InputListener &&other) = default;
			InputListener& operator=(const InputListener &other) = delete;
			InputListener& operator=(InputListener &&other) = default;
			~InputListener() = default;

			/** \brief Returns true if there are no elements in the input queue.
			 */
			bool isEmpty() const noexcept;

			/** \brief Returns next line read by the listener.
			 *	Waits until a new line is available, then returns it and removes it from the listener queue.
			 */
			std::string getLine();

			/** \brief Peeks into the next line read by the listener.
			 *	Waits until a new line is available, then returns it but does not remove from the listener queue.
			 *  @see getLine()
			 */
			std::string peekLine();

			/** \brief Removes next line read by the listener queue.
			 *	Waits until new line is available, then removes it from the listener queue without returning it.
			 */
			void consumeLine();

			/** \brief Removes next line read by the listener queue.
			 *	Waits until new line is available, then removes it from the listener queue without returning, but checks if the consumed line is the same as specified in the parameter.
			 */
			void consumeLine(const std::string &line);

			/** \brief Adds a line to the listener queue.
			 *	Can be used to add specific input to the listener queue, bypassing the input stream.
			 */
			void pushLine(const std::string &line);

		private:
			std::string wait_for_line(bool consume_result);
			std::string read_line_from_stream();
	};

	class OutputSender
	{
		private:
			std::ostream *output_stream = nullptr;
			mutable std::mutex sender_mutex;
		public:
			OutputSender() = default;
			OutputSender(std::ostream &outputStream);
			void send(const std::string &msg) const;
	};

	struct NoData
	{
	};
	struct Option
	{
			std::string name;
			std::string value;
	};
	struct SearchSummary
	{
			Node node;
			std::vector<Move> principal_variation;
			double time_used = 0.0;
			int number_of_nodes = 0;
	};

	/**
	 * Enumeration used to tell what to do with data received from user or engine.
	 */
	enum class MessageType
	{
		EMPTY_MESSAGE, /**< empty message with no data */
		START_PROGRAM, /**< GUI -> Engine : send to initialize program */
		SET_OPTION, /**< GUI -> Engine : send with option name and its value (as string) */
		SET_POSITION, /**< GUI -> Engine : used to set specific position */
		START_SEARCH, /**< GUI -> Engine : send to start searching with specific options what to do after the search ends */
		STOP_SEARCH, /**< GUI -> Engine : send to immediately stop search */
		EXIT_PROGRAM, /**< GUI -> Engine : send to exit the program */
		ABOUT_ENGINE, /**< GUI -> Engine : request to send back information about the engine */
		PLAIN_STRING, /**< GUI <- Engine message containing string with no specific structure */
		UNKNOWN_COMMAND, /**< GUI <- Engine : used as a response after unknown message from user */
		ERROR, /**< GUI <- Engine : used as a response after an error */
		INFO_MESSAGE, /**< GUI <- Engine : used to send some information string from engine */
		BEST_MOVE, /**< GUI <- Engine : used to send move made by the engine */
	};
	class Message
	{
		private:
			MessageType type;
			std::variant<NoData, GameConfig, std::vector<Move>, Option, Move, std::string, SearchSummary> data;
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
			bool holdsSearchSummary() const noexcept;

			MessageType getType() const noexcept;
			GameConfig getGameConfig() const;
			std::vector<Move> getListOfMoves() const;
			Option getOption() const;
			Move getMove() const;
			std::string getString() const;
			SearchSummary getSearchSummary() const;

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
			const Message& peek() const;
	};

	enum class ProtocolType
	{
		GOMOCUP, /**< protocol used in Gomocup tournament */
		EXTENDED_GOMOCUP, /**< protocol derived from the base one, extended with several new commands */
		YIXINBOARD /**< protocol used by YixinBoard GUI */
	};
	std::string toString(ProtocolType pt);
	ProtocolType protocolFromString(const std::string &str);

	class ProtocolRuntimeException: public std::runtime_error
	{
		public:
			ProtocolRuntimeException(const char *msg) :
					std::runtime_error(msg)
			{
			}
			ProtocolRuntimeException(const std::string &msg) :
					std::runtime_error(msg)
			{
			}
	};

	/**
	 * @brief Class responsible for translating commands from GUI into set of messages for the search engine.
	 */
	class Protocol
	{
		protected:
			mutable std::recursive_mutex protocol_mutex;
			MessageQueue &input_queue;
			MessageQueue &output_queue;
			std::vector<std::pair<std::string, std::function<void(InputListener&)>>> input_processors;
			std::vector<std::pair<MessageType, std::function<void(OutputSender&)>>> output_processors;
		public:
			Protocol(MessageQueue &queueIN, MessageQueue &queueOUT);
			Protocol(const Protocol &other) = delete;
			Protocol(Protocol &&other) = delete;
			Protocol& operator=(const Protocol &other) = delete;
			Protocol& operator=(Protocol &&other) = delete;
			virtual ~Protocol() = default;

			virtual void reset() = 0;
			virtual ProtocolType getType() const noexcept = 0;

			void processInput(InputListener &listener);
			void processOutput(OutputSender &sender);

			void registerInputProcessor(const std::string &cmd, const std::function<void(InputListener&)> &function);
			void registerOutputProcessor(MessageType type, const std::function<void(OutputSender&)> &function);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_PROTOCOL_HPP_ */
