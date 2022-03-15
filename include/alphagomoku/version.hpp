/*
 * version.hpp
 *
 *  Created on: May 17, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_VERSION_HPP_
#define ALPHAGOMOKU_VERSION_HPP_

#include <string>

namespace ag
{
	struct Version
	{
			static const int major = 5;
			static const int minor = 2;
			static const int revision = 9;
	};
	class ProgramInfo
	{
		public:
			static std::string name()
			{
				return "AlphaGomoku";
			}
			static std::string version()
			{
				return std::to_string(Version::major) + '.' + std::to_string(Version::minor) + '.' + std::to_string(Version::revision);
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
			static std::string copyright()
			{
				return "Copyright (C) 2017-2022 " + author();
			}
			static std::string build()
			{
				return "Build on " + std::string(__DATE__) + " at " + std::string(__TIME__);
			}
			static std::string license()
			{
				return "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.";
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_VERSION_HPP_ */
