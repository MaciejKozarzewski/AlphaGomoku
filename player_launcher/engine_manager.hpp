/*
 * engine_manager.hpp
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
#include <fstream>
#include <future>

namespace ag
{
	/* implemented in "configuration.cpp" */
	void createConfig(const Json &benchmarkResults);

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
			ResourceManager resource_manager;
			std::future<void> input_future;
			std::future<void> search_future;
			std::ofstream logfile;
			bool is_running = true;

			Json config;

			bool display_help = false;
			bool display_version = false;
			bool run_benchmark = false;
			bool run_configuration = false;
			std::string name_of_config = "config.json";
		public:
			EngineManager();

			void processArguments(int argc, char *argv[]);
			void run();
		private:
			void create_arguments();
			void help() const;
			void version() const;
			void benchmark() const;
			void configure();
			void load_config(const std::string &path);
			void prepare_config();

			void setup_protocol();
			void process_input_from_user();
			void setup_logging();
	};

} /* namespace ag */

#endif /* ENGINE_MANAGER_HPP_ */
