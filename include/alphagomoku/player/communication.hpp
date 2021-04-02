/*
 * communication.hpp
 *
 *  Created on: Apr 2, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_COMMUNICATION_HPP_
#define ALPHAGOMOKU_PLAYER_COMMUNICATION_HPP_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace ag
{

	class InputListener
	{
		private:
			std::queue<std::string> input_queue;
			mutable std::mutex listener_mutex;
			std::condition_variable listener_cond;
			std::thread listener_thread;
			bool is_running = true;

		public:
			InputListener();
			~InputListener();
			bool isEmpty() const noexcept;

			std::string getLine();
			void push(const std::string &line);

			void run();
	};

	class OutputSender
	{
		public:
			void send(const std::string &msg);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_COMMUNICATION_HPP_ */
