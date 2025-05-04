/*
 * NetworkLoader.cpp
 *
 *  Created on: Apr 28, 2025
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/NetworkLoader.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <minml/graph/swa_utils.hpp>

namespace
{
	using namespace ag;

	std::unique_ptr<AGNetwork> load_and_optimize(const std::string &path, bool optimize)
	{
		std::unique_ptr<AGNetwork> result = loadAGNetwork(path);
		if (optimize)
			result->optimize(1);
		return result;
	}
}

namespace ag
{
	NetworkLoader::NetworkLoader(const char *path) :
			NetworkLoader(std::string(path))
	{
	}
	NetworkLoader::NetworkLoader(const std::string &path) :
			paths( { path })
	{
	}
	NetworkLoader::NetworkLoader(const std::vector<std::string> &path) :
			paths(path)
	{
	}
	std::unique_ptr<AGNetwork> NetworkLoader::get(bool optimized) const
	{
		if (paths.empty())
			return nullptr;
		std::unique_ptr<AGNetwork> result = load_and_optimize(paths.at(0), optimized);
		for (size_t i = 1; i < paths.size(); i++)
		{
			std::unique_ptr<AGNetwork> network = load_and_optimize(paths.at(i), optimized);
			const float alpha = 1.0f / (i + 1);
			ml::averageModelWeights(alpha, network->get_graph(), 1.0f - alpha, result->get_graph());
		}
		return result;
	}
}

