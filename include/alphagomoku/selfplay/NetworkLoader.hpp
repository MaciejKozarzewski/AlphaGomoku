/*
 * NetworkLoader.hpp
 *
 *  Created on: Apr 28, 2025
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_NETWORKLOADER_HPP_
#define ALPHAGOMOKU_SELFPLAY_NETWORKLOADER_HPP_

#include <vector>
#include <string>
#include <memory>

namespace ag
{
	class AGNetwork;
}

namespace ag
{
	class NetworkLoader
	{
			std::vector<std::string> paths;
		public:
			NetworkLoader() noexcept = default;
			NetworkLoader(const char *path);
			NetworkLoader(const std::string &path);
			NetworkLoader(const std::vector<std::string> &path);
			std::unique_ptr<AGNetwork> get(bool optimized = true) const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_NETWORKLOADER_HPP_ */
