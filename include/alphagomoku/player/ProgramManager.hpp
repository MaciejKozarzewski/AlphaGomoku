/*
 * ProgramManager.hpp
 *
 *  Created on: Aug 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ENGINEMANAGER_HPP_
#define ENGINEMANAGER_HPP_

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/utils/ArgumentParser.hpp>
#include <iostream>
#include <fstream>
#include <future>

namespace ag
{
	/* implemented in "configuration.cpp" */
	void createConfig(const Json &benchmarkResults);

	/* implemented in "benchmark.cpp" */
	Json run_benchmark(const std::string &path_to_network, const OutputSender &output_sender);

	/**
	 * @brief Main class that interfaces between the user and the search engine.
	 *
	 * It parses and executes command line arguments (if any). Uses Protocol instance to translate runtime commands from user
	 * into search commands, parses them and calls appropriate search engine methods.
	 *
	 * Implemented in "ProgramManager.cpp"
	 */
	class ProgramManager
	{
		private:
			ArgumentParser argument_parser;
			InputListener input_listener;
			OutputSender output_sender;

			MessageQueue input_queue;
			MessageQueue output_queue;
			std::unique_ptr<Protocol> protocol;
			std::unique_ptr<SearchEngine> search_engine;
			std::unique_ptr<EngineSettings> engine_settings;
			std::unique_ptr<EngineController> engine_controller;
			TimeManager time_manager;

			std::future<void> input_future;
			std::future<void> search_future;
			std::ofstream logfile;
			std::string name_of_config_file = "config.json";

			Json config;

			bool display_help = false;
			bool display_version = false;
			bool run_benchmark = false;
			bool run_configuration = false;
			bool is_running = true;

			int game_counter = 0;

			matrix<Sign> board;
			Sign sign_to_move = Sign::NONE;
		public:
			ProgramManager();

			bool processArguments(int argc, char *argv[]);
			void run();
		private:
			void create_arguments();
			void help() const;
			void version() const;
			void benchmark() const;
			void configure();
			bool load_config(const std::string &path);
			void prepare_config();

			void setup_protocol();
			void process_input_from_user();
			void setup_logging();
			void setup_engine();
			void set_position(const std::vector<Move> &listOfMoves);
			void setup_controller(const std::string &type);
			bool is_game_config_correct() const;
	};

} /* namespace ag */

#endif /* ENGINEMANAGER_HPP_ */
