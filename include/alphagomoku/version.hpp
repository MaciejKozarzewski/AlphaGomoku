/*
 * version.hpp
 *
 *  Created on: 17 maj 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VERSION_HPP_
#define ALPHAGOMOKU_VERSION_HPP_

#include <string>

namespace ag
{
#define VERSION_MAJOR 5
#define VERSION_MINOR 1
#define VERSION_PATCH 0

	class ProgramInfo
	{
		public:
			static std::string name()
			{
				return "AlphaGomoku";
			}
			static std::string version()
			{
				return std::to_string(VERSION_MAJOR) + '.' + std::to_string(VERSION_MINOR) + '.' + std::to_string(VERSION_PATCH);
			}
			static std::string author()
			{
				return "Maciej Kozarzewski";
			}
			static std::string country()
			{
				return "Poland";
			}
			static std::string website()
			{
				return "https://github.com/MaciejKozarzewski/AlphaGomoku";
			}
			static std::string email()
			{
				return "alphagomoku.mk@gmail.com";
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VERSION_HPP_ */
