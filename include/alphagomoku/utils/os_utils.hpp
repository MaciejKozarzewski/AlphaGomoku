/*
 * os_utils.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_OS_UTILS_HPP_
#define ALPHAGOMOKU_UTILS_OS_UTILS_HPP_

#include <string>

namespace ag
{
	/*
	 * \brief Returns path to the executable including executable name
	 */
	std::string getExecutablePath();

	/*
	 * \brief Parses the path to extract path and executable name.
	 */
	std::pair<std::string, std::string> parseLaunchPath(std::string text);

	enum class PrefetchMode
	{
		READ,
		WRITE
	};
	template<PrefetchMode Mode, int LocalityHint>
	void prefetchMemory(const void *ptr) noexcept
	{
#if (defined(__GNUC__) && defined(__cplusplus)) || defined(__clang__)
		__builtin_prefetch(ptr, static_cast<int>(Mode), LocalityHint);
#elif defined(_MSC_VER)
		_mm_prefetch(ptr, LocaliTyHint);
#else

#endif
	}

	enum class SignalType
	{
		INT,
		ILL,
		ABRT,
		FPE,
		SEGV,
		TERM
	};
	enum class SignalHandlerMode
	{
		DEFAULT,
		IGNORE,
		CUSTOM
	};
	void setupSignalHandler(SignalType type, SignalHandlerMode mode);
	bool hasCapturedSignal(SignalType type) noexcept;

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_OS_UTILS_HPP_ */
