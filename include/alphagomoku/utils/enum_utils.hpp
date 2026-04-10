/*
 * enum_utils.hpp
 *
 *  Created on: Apr 23, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_ENUM_UTILS_HPP_
#define ALPHAGOMOKU_UTILS_ENUM_UTILS_HPP_

#include <string>

namespace ag
{
	template<class EC>
	std::string enumToString(EC e)
	{
		return std::string();
	}
	template<class EC>
	EC enumFromString(const std::string &str)
	{
		return EC { };
	}
}
/* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_ENUM_UTILS_HPP_ */
