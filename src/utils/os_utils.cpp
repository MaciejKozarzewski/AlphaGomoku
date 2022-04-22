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
} /* namespace ag */

