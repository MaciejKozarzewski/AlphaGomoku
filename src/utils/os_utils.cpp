/*
 * os_utils.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/os_utils.hpp>

#include <algorithm>
#include <stdexcept>
#include <csignal>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <limits.h>
#  include <unistd.h>
#endif

namespace
{
	volatile sig_atomic_t sigint_status;
	volatile sig_atomic_t sigill_status;
	volatile sig_atomic_t sigabrt_status;
	volatile sig_atomic_t sigfpe_status;
	volatile sig_atomic_t sigsegv_status;
	volatile sig_atomic_t sigterm_status;

	void sigint_handler(int status)
	{
		sigint_status = status;
	}
	void sigill_handler(int status)
	{
		sigill_status = status;
	}
	void sigabrt_handler(int status)
	{
		sigabrt_status = status;
	}
	void sigfpe_handler(int status)
	{
		sigfpe_status = status;
	}
	void sigsegv_handler(int status)
	{
		sigsegv_status = status;
	}
	void sigterm_handler(int status)
	{
		sigterm_status = status;
	}
	int get_signal(ag::SignalType st)
	{
		switch (st)
		{
			case ag::SignalType::INT:
				return SIGINT;
			case ag::SignalType::ILL:
				return SIGILL;
			case ag::SignalType::ABRT:
				return SIGABRT;
			case ag::SignalType::FPE:
				return SIGFPE;
			case ag::SignalType::SEGV:
				return SIGSEGV;
			case ag::SignalType::TERM:
				return SIGTERM;
			default:
				throw std::logic_error("Unknown signal type " + std::to_string((int) st));
		}
	}
}

namespace ag
{
	std::string get_executable_path()
	{
#if defined(_WIN32)
		char path[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, path, MAX_PATH);
		return std::string(path);
#elif defined(__linux__)
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		return std::string(result, (count > 0) ? count : 0);
#endif
	}

	std::pair<std::string, std::string> parse_launch_path(std::string text)
	{
		if (text.find('\\') == std::string::npos and text.find('/') == std::string::npos) // no slashes
			return std::pair<std::string, std::string>( { "", text });
		size_t last_slash = text.length();
		for (size_t i = 0; i < text.length(); i++)
			if (text[i] == '\\' or text[i] == '/')
			{
#if defined(_WIN32)
				text[i] = '\\';
#elif defined(__linux__)
				text[i] = '/';
#endif
				last_slash = i + 1;
			}
		return std::pair<std::string, std::string>( { text.substr(0, last_slash), text.substr(last_slash) });
	}

	void setupSignalHandler(SignalType type, SignalHandlerMode mode)
	{
		switch (mode)
		{
			case SignalHandlerMode::DEFAULT:
				signal(get_signal(type), SIG_DFL);
				break;
			case SignalHandlerMode::IGNORE:
				signal(get_signal(type), SIG_IGN);
				break;
			case SignalHandlerMode::CUSTOM:
			{
				switch (type)
				{
					case SignalType::INT:
						signal(SIGINT, sigint_handler);
						break;
					case SignalType::ILL:
						signal(SIGILL, sigill_handler);
						break;
					case SignalType::ABRT:
						signal(SIGABRT, sigabrt_handler);
						break;
					case SignalType::FPE:
						signal(SIGFPE, sigfpe_handler);
						break;
					case SignalType::SEGV:
						signal(SIGSEGV, sigsegv_handler);
						break;
					case SignalType::TERM:
						signal(SIGTERM, sigterm_handler);
						break;
					default:
						throw std::logic_error("Unknown signal type " + std::to_string((int) type));
				}
				break;
			}
			default:
				throw std::logic_error("Unknown signal handler mode " + std::to_string((int) mode));
		}
	}
	bool hasCapturedSignal(SignalType type) noexcept
	{
		switch (type)
		{
			case SignalType::INT:
				return sigint_status == SIGINT;
			case SignalType::ILL:
				return sigill_status == SIGILL;
			case SignalType::ABRT:
				return sigabrt_status == SIGABRT;
			case SignalType::FPE:
				return sigfpe_status == SIGFPE;
			case SignalType::SEGV:
				return sigsegv_status == SIGSEGV;
			case SignalType::TERM:
				return sigterm_status == SIGTERM;
			default:
				return false; // unknown signal type
		}
	}
} /* namespace ag */

