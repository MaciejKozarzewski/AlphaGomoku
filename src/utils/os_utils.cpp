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
		wchar_t path[MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, path, MAX_PATH);
		std::string result(MAX_PATH, 0);
		std::transform(path, path + MAX_PATH, result.begin(), [](wchar_t c)
		{	return static_cast<char>(c);});
		return result;
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

