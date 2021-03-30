/*
 * GomocupPlayer.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/GomocupPlayer.hpp>
#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <filesystem>

namespace ag
{

	GomocupPlayer::GomocupPlayer(const std::string &launch_path)
	{
	}
	GomocupPlayer::~GomocupPlayer()
	{
		if (search_engine != nullptr)
			search_engine->stop();
	}
	void GomocupPlayer::run()
	{
		while (is_running)
		{
			std::string line = getLine();
			if (config.use_logging)
				std::clog << "Got: " << line << '\n';
			processInput(line);
		}
	}

	void GomocupPlayer::processInput(const std::string &msg)
	{
		if (startsWith(msg, "INFO"))
		{
			INFO(msg);
			return;
		}
		if (startsWith(msg, "START"))
		{
			START(msg);
			return;
		}
		if (startsWith(msg, "RECTSTART"))
		{
			RECTSTART(msg);
			return;
		}
		if (startsWith(msg, "RESTART"))
		{
			RESTART();
			return;
		}
		if (startsWith(msg, "SWAP2BOARD"))
		{
			SWAP2BOARD();
			return;
		}
		if (startsWith(msg, "BEGIN"))
		{
			BEGIN();
			return;
		}
		if (startsWith(msg, "BOARD"))
		{
			BOARD();
			return;
		}
		if (startsWith(msg, "TURN"))
		{
			TURN(msg);
			return;
		}
		if (startsWith(msg, "TAKEBACK"))
		{
			TAKEBACK(msg);
			return;
		}
		if (startsWith(msg, "END"))
		{
			END();
			return;
		}
		if (startsWith(msg, "PLAY"))
		{
			PLAY(msg);
			return;
		}
		if (startsWith(msg, "ABOUT"))
		{
			ABOUT();
			return;
		}
		if (startsWith(msg, "YXSHOWINFO"))
		{
			YXSHOWINFO();
			return;
		}
		return UNKNOWN(msg);
	}
	void GomocupPlayer::respondWith(const std::string &answer)
	{
		std::clog << "Answered: " << answer << '\n';
		printLine(answer);
	}

	void GomocupPlayer::INFO(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		if (tmp[1] == "timeout_turn")
		{
			config.info_timeout_turn = std::stoll(tmp[2]);
			return;
		}
		if (tmp[1] == "timeout_match")
		{
			config.info_timeout_match = std::stoll(tmp[2]);
			return;
		}
		if (tmp[1] == "time_left")
		{
			config.info_time_left = std::stoll(tmp[2]);
			return;
		}
		if (tmp[1] == "max_memory")
		{
			config.info_max_memory = std::stoll(tmp[2]);
			return;
		}
		if (tmp[1] == "game_type")
		{
			config.info_game_type = std::stoi(tmp[2]);
			return;
		}
		if (tmp[1] == "rule")
		{
			switch (std::stoi(tmp[2]))
			{
				case 0:
					config.game_config.rules = GameRules::FREESTYLE;
					break;
				case 1:
					config.game_config.rules = GameRules::STANDARD;
					break;
				case 2:
					break; // continuous game - not supported
				case 4:
					config.game_config.rules = GameRules::RENJU;
					break;
				case 8:
					config.game_config.rules = GameRules::CARO; // extending Gomocup protocol
					break;
			}
			return;
		}
		if (tmp[1] == "evaluate")
		{
			return;
		}
		if (tmp[1] == "folder")
		{
			config.info_folder = tmp[2];
			return;
		}
	}
	void GomocupPlayer::START(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		config.game_config.rows = std::stoi(tmp[1]);
		config.game_config.cols = std::stoi(tmp[1]);

		if (config.game_config.rows == 15 || config.game_config.rows == 20)
		{
			last_time = getTime();
			respondWith("OK");
		}
		else
			ERROR("unsupported board size");
	}
	void GomocupPlayer::RECTSTART(const std::string &msg)
	{
		ERROR("rectangular board is not supported");
	}
	void GomocupPlayer::RESTART()
	{
		getSearchEngine().stop();
		getGame().beginGame();

		last_time = getTime();
		respondWith("OK");
	}
	void GomocupPlayer::SWAP2BOARD()
	{
		getGame().beginGame();
		std::string line1 = getLine();
		if (line1 != "DONE") // place first three stones
		{
			std::string line2 = getLine();
			std::string line3 = getLine();
			getGame().makeMove(moveFromString(line1, Sign::CROSS));
			getGame().makeMove(moveFromString(line2, Sign::CIRCLE));
			getGame().makeMove(moveFromString(line3, Sign::CROSS));

			std::string line4 = getLine();
			if (line4 != "DONE")
			{
				// 5 stones were placed
				std::string line5 = getLine();
				std::string line6 = getLine(); // DONE
				assert(line6 == "DONE");
				getGame().makeMove(moveFromString(line4, Sign::CIRCLE));
				getGame().makeMove(moveFromString(line5, Sign::CROSS));
			}
		}
		getSearchEngine().setState(getGame());
		getSearchEngine().swap2();
	}
	void GomocupPlayer::BEGIN()
	{
		getGame().beginGame();
		getSearchEngine().setState(getGame());
		getSearchEngine().makeMove();
	}
	void GomocupPlayer::BOARD()
	{
		matrix<Sign> board(config.game_config.rows, config.game_config.cols);
		Sign my_sign = Sign::NONE;
		while (true)
		{
			std::string line = getLine();
			if (line == "DONE")
				break;
			auto tmp = split(line, ',');
			int x = std::stoi(tmp[1]);
			int y = std::stoi(tmp[0]);
			int sign = std::stoi(tmp[2]);
			if (my_sign == Sign::NONE)
			{
				if (sign == 1) //own stone
					my_sign = Sign::CROSS;
				else
					my_sign = Sign::CIRCLE;
			}
			board.at(x, y) = (sign == 1) ? my_sign : invertSign(my_sign);
		}

		getGame().beginGame();
		getGame().setBoard(board, my_sign);
		getSearchEngine().setState(getGame());
		getSearchEngine().makeMove();
	}
	void GomocupPlayer::TURN(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		Move move = moveFromString(tmp[1], getGame().getSignToMove());
		getGame().makeMove(move);
		getSearchEngine().setState(getGame());
		getSearchEngine().makeMove();
	}
	void GomocupPlayer::TAKEBACK(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		Move move = moveFromString(tmp[1], Sign::NONE);

		move.sign = getGame().getBoard().at(move.row, move.col);

		getSearchEngine().stop();
		getGame().makeMove(move);
		respondWith("OK");
	}
	void GomocupPlayer::END()
	{
		is_running = false;
	}

	void GomocupPlayer::SUGGEST()
	{
		ERROR("SUGGEST not supported");
	}
	void GomocupPlayer::PLAY(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		Move move = moveFromString(tmp[1], getGame().getSignToMove());
		getSearchEngine().stop();
		getGame().makeMove(move);
		getSearchEngine().ponder();
	}

	void GomocupPlayer::ABOUT()
	{
		std::string result;
		result += "name=\"AlphaGomoku\", ";
		result += "version=\"4.1\", ";
		result += "author=\"Maciej Kozarzewski\", ";
		result += "country=\"Poland\", ";
		result += "www=\"https://github.com/MaciejKozarzewski/AlphaGomoku\", ";
		result += "email=\"alphagomoku.mk@gmail.com\"";
		respondWith(result);
	}
	void GomocupPlayer::YXSHOWINFO()
	{
		config.is_using_yixin_board = true;
		respondWith("OK");
	}

	void GomocupPlayer::ERROR(const std::string &msg)
	{
		if (msg.length() == 0)
			respondWith("ERROR");
		else
			respondWith(std::string("ERROR ") + msg);
	}
	void GomocupPlayer::MESSAGE(const std::string &msg)
	{
		respondWith(std::string("MESSAGE ") + msg);
	}
	void GomocupPlayer::UNKNOWN(const std::string &msg)
	{
		respondWith(std::string("UNKNOWN ") + msg);
	}

	Game& GomocupPlayer::getGame()
	{
		if (game == nullptr)
		{
			if (config.game_config.rows != config.game_config.cols)
				ERROR("board is not square");
			if (config.game_config.rules != GameRules::FREESTYLE and config.game_config.rules != GameRules::STANDARD)
				ERROR("only freestyle and standard rules are supported");
			if (config.game_config.rules == GameRules::FREESTYLE and config.game_config.rows != 20)
				ERROR("freestyle rules is supported only on 20x20 board");
			if (config.game_config.rules == GameRules::STANDARD and config.game_config.rows != 15)
				ERROR("standard rules is supported only on 15x15 board");
			game = std::make_unique<Game>(config.game_config);
		}
		return *game;
	}
	SearchEngine& GomocupPlayer::getSearchEngine()
	{
		if (search_engine == nullptr)
		{
			Json cfg = loadConfig();
			search_engine = std::make_unique<SearchEngine>(cfg, config.game_config, *this);
		}
		return *search_engine;
	}

	Json GomocupPlayer::loadConfig()
	{
		if (not std::filesystem::exists(local_launch_path + "config.json"))
			createDefaultConfig();
		FileLoader fl(local_launch_path + "config.json");
		return fl.getJson();
	}
	void GomocupPlayer::createDefaultConfig() const
	{
		Json result;
		result["search_options"] = SearchConfig::getDefault();
		result["tree_options"] = TreeConfig::getDefault();
		result["cache_options"] = CacheConfig::getDefault();
		FileSaver fs(local_launch_path + "config.json");
		fs.save(result, SerializedObject(), 2);
	}

} /* namespace ag */

