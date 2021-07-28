/*
 * configuration.hpp
 *
 *  Created on: Jun 11, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include <string>

class Json;

namespace ag
{

	Json loadConfig(const std::string &localLaunchPath);
	void createConfig(const std::string &localLaunchPath);

} /* namespace ag */


#endif /* CONFIGURATION_HPP_ */
