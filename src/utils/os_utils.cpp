/*
 * os_utils.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/os_utils.hpp>

#include <algorithm>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <limits.h>
#  include <unistd.h>
#endif

namespace ag
{
#if defined(_WIN32)
	std::string get_executable_path()
	{
		char path[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, path, MAX_PATH);
		return std::string(path);
	}
#elif defined(__linux__)
	std::string get_executable_path()
	{
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		return std::string(result, (count > 0) ? count : 0);
	}
#endif

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
} /* namespace ag */

