/*
 * Logger.cpp
 *
 *  Created on: Apr 4, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/Logger.hpp>

#include <assert.h>

namespace ag
{
	void Logger::enable() noexcept
	{
		std::lock_guard lock(get_instance().logger_mutex);
		get_instance().is_enabled = true;
	}
	void Logger::disable() noexcept
	{
		std::lock_guard lock(get_instance().logger_mutex);
		get_instance().is_enabled = false;
	}
	void Logger::write(const std::string &str, bool flushAfterWrite)
	{
		std::lock_guard lock(get_instance().logger_mutex);
		if (get_instance().is_enabled)
		{
			assert(get_instance().output_stream != nullptr);
			get_instance().output_stream->write(str.data(), str.size());
			if (flushAfterWrite)
				get_instance().output_stream->operator <<(std::endl);
		}
	}
	void Logger::redirectTo(std::ostream &stream)
	{
		std::lock_guard lock(get_instance().logger_mutex);
		get_instance().output_stream = &stream;
	}
	Logger& Logger::get_instance() noexcept
	{
		static Logger logger;
		return logger;
	}

} /* namespace ag */

