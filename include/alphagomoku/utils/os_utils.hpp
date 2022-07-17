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
} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_UTILS_OS_UTILS_HPP_ */
