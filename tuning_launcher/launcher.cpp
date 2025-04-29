/*
 * launcher.cpp
 *
 *  Created on: Apr 25, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/ArgumentParser.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/NetworkLoader.hpp>

#include <minml/utils/json.hpp>

using namespace ag;

int main(int argc, char *argv[])
{
	std::string config_path;
	std::string output_path;

	ArgumentParser ap;
	ap.addArgument("config_path", [&](const std::string &arg)
	{	config_path = arg;});
	ap.addArgument("output_path", [&](const std::string &arg)
	{	output_path = arg;});
	ap.parseArguments(argc, argv);

	const Json config = FileLoader(config_path).getJson();

	const GameConfig game_config(config["game_config"]);
	const EvaluationConfig evaluation_config(config["evaluation_config"]);

	const SelfplayConfig base_player_config(config["base_player_config"]);
	const SelfplayConfig tested_player_config = evaluation_config.selfplay_options;

	EvaluationManager manager(game_config, evaluation_config.selfplay_options);

	const std::string network_path = config["network_path"].getString();

	manager.setFirstPlayer(base_player_config, network_path, "base");
	manager.setSecondPlayer(tested_player_config, network_path, "tested");

	const double start = getTime();
	manager.generate(evaluation_config.selfplay_options.games_per_iteration);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';

	const std::string to_save = manager.getPGN();
	std::ofstream file(output_path, std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();

	return 0;
}

