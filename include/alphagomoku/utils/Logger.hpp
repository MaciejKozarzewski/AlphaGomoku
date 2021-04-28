/*
 * Logger.hpp
 *
 *  Created on: 4 kwi 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_LOGGER_HPP_
#define ALPHAGOMOKU_UTILS_LOGGER_HPP_

#include <string>
#include <iostream>
#include <mutex>

namespace ag
{

	class Logger
	{
		private:
			std::mutex logger_mutex;
			std::ostream *output_stream = &(std::clog);
			bool is_enabled = false;
		public:
			static void enable() noexcept;
			static void disable() noexcept;

			static void write(const std::string &str, bool flushAfterWrite = true);
			static void redirectTo(std::ostream &stream);
		private:
			static Logger& get_instance() noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_LOGGER_HPP_ */
