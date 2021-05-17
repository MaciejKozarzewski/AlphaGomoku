/*
 * version.hpp
 *
 *  Created on: 17 maj 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_VERSION_HPP_
#define ALPHAGOMOKU_VERSION_HPP_

#include <string>

namespace ag
{
#define VERSION_MAJOR 5
#define VERSION_MINOR 0
#define VERSION_PATCH 1

	std::string version_to_string()
	{
		return std::to_string(VERSION_MAJOR) + '.' + std::to_string(VERSION_MINOR) + '.' + std::to_string(VERSION_PATCH);
	}
} /* namespace ag */

#endif /* ALPHAGOMOKU_VERSION_HPP_ */
