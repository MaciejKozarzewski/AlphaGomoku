/*
 * GomocupPlayer.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_GOMOCUPPLAYER_HPP_
#define ALPHAGOMOKU_PLAYER_GOMOCUPPLAYER_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <inttypes.h>
#include <memory>
#include <string>
#include <iostream>

namespace ag
{
	struct GomocupPlayerConfig
	{
			GameConfig game_config;

			uint64_t info_max_memory = 100000000; // memory limit (bytes, 0=no limit)
			uint64_t info_timeout_turn = 5000; // time limit for each move (milliseconds, 0=play as fast as possible)
			uint64_t info_timeout_match = 100000; // time limit of a whole match (milliseconds, 0=no limit)
			uint64_t info_time_left = 100000; // remaining time limit of a whole match (milliseconds)

			int info_game_type = 1; // 0=opponent is human, 1=opponent is brain, 2=tournament, 3=network tournament
			std::string info_folder; // folder for persistent files

			bool is_using_yixin_board = false;

			// non Gomocup options
			bool use_logging = false;
	};

	class GomocupPlayer
	{
		private:
			std::string local_launch_path;
			std::istream &input_stream;
			std::ostream &output_stream;
			std::ostream &logging_stream;

			bool is_running = true;

			double last_time = 0.0;
			GomocupPlayerConfig config;

			matrix<Sign> board;
			Sign sign_to_move = Sign::NONE;
			std::unique_ptr<SearchEngine> search_engine;

		public:
			GomocupPlayer(const std::string &launchPath, std::istream &inputStream = std::cin, std::ostream &outputStream = std::cout,
					std::ostream &loggingStream = std::clog);
			~GomocupPlayer();
			bool isRunning() const noexcept;
			void processInput();
			void respondWith(const std::string &answer);

			void makeMove(Move move);
			GomocupPlayerConfig getConfig() const noexcept;
			SearchEngine& getSearchEngine();
			matrix<Sign>& getBoard();

		private:
			void INFO(const std::string &msg);
			void START(const std::string &msg);
			void RECTSTART(const std::string &msg);
			void RESTART();
			void SWAP2BOARD();
			void BEGIN();
			void BOARD();
			void TURN(const std::string &msg);
			void TAKEBACK(const std::string &msg);
			void END();

			void SUGGEST();
			void PLAY(const std::string &msg);

			void ABOUT();
			void YXSHOWINFO();

			void ERROR(const std::string &msg);
			void MESSAGE(const std::string &msg);
			void UNKNOWN(const std::string &msg);

			Json loadConfig();
			void createDefaultConfig() const;

			std::string get_line();
			void print_line(const std::string &line);
			void log_line(const std::string &line);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_GOMOCUPPLAYER_HPP_ */