/*
 * engine_settings.hpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ENGINE_MANAGER_HPP_
#define ENGINE_MANAGER_HPP_

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/argument_parser.hpp>

#include <iostream>

namespace ag
{
	/* implemented in "configuration.cpp" */
	Json loadConfig(const std::string &path);
	void createConfig(const std::string &path);

	/* implemented in "benchmark.cpp" */
	Json run_benchmark(const std::string &path_to_network, const OutputSender &output_sender);

	/* implemented in "engine_manager.cpp" */
	class EngineManager
	{
		private:
			ArgumentParser argument_parser;
			InputListener input_listener;
			OutputSender output_sender;

			MessageQueue input_queue;
			MessageQueue output_queue;
			std::unique_ptr<Protocol> protocol;
			std::unique_ptr<SearchEngine> search_engine;
			GameConfig game_config;
			ResourceManager resource_manager;

			Json config;

			bool display_help = false;
			bool display_version = false;
			bool run_benchmark = false;
			bool run_configuration = false;
			std::string name_of_config = "config.json";
		public:
			EngineManager();

			void processArguments(int argc, char *argv[]);
		private:
			void create_arguments();

			void print_help_and_exit() const;
			void print_version_and_exit() const;
			void run_benchmark_and_exit() const;
			void run_configuration_and_exit();
			void load_config(const std::string &path);
	};

} /* namespace ag */

#endif /* ENGINE_MANAGER_HPP_ */
