/*
 * os_utils.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_UTILS_OS_UTILS_HPP_
#define INCLUDE_ALPHAGOMOKU_UTILS_OS_UTILS_HPP_

#include <string>

namespace ag
{
	/*
	 * \brief Returns path to the executable including executable name
	 */
	std::string get_executable_path();

	/*
	 * \brief Parses the path to extract path and executable name.
	 */
	std::pair<std::string, std::string> parse_launch_path(std::string text);

	enum class PrefetchMode
	{
		READ,
		WRITE
	};
	template<PrefetchMode mode, int localityHint>
	void prefetch_memory(const void *ptr) noexcept
	{
#if (defined(__GNUC__) && defined(__cplusplus)) || defined(__clang__)
		__builtin_prefetch(ptr, static_cast<int>(mode), localityHint);
#elif defined(_MSC_VER)
		_mm_prefetch(ptr, localiTyHint);
#else

#endif
	}

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_UTILS_OS_UTILS_HPP_ */
