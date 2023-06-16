/*
 * selfcheck.hpp
 *
 *  Created on: Jun 13, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_SELFCHECK_HPP_
#define ALPHAGOMOKU_UTILS_SELFCHECK_HPP_

#include <iostream>

namespace ag
{
	void checkMLBackend(std::ostream &stream);
	void checkNeuralNetwork(std::ostream &stream);
	void checkPatternCalculation(std::ostream &stream);

	void checkConfigFile(std::ostream &stream, const std::string &pathToConfig);
	void checkIntegrity(std::ostream &stream, const std::string &launchPath);

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_SELFCHECK_CPP_ */
