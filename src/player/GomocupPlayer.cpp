/*
 * GomocupPlayer.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <alphagomoku/player/GomocupPlayer.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <filesystem>

namespace ag
{

	GomocupPlayer::GomocupPlayer(const std::string &launchPath, std::istream &inputStream, std::ostream &outputStream, std::ostream &loggingStream) :
			local_launch_path(launchPath),
			input_stream(inputStream),
			output_stream(outputStream),
			logging_stream(loggingStream)
	{
	}
	GomocupPlayer::~GomocupPlayer()
	{
		if (search_engine != nullptr)
			search_engine->stop();
	}
	bool GomocupPlayer::isRunning() const noexcept
	{
		return is_running;
	}

	void GomocupPlayer::processInput()
	{
		std::string line = get_line();
		log_line("Got: " + line);

		if (startsWith(line, "INFO"))
		{
			INFO(line);
			return;
		}
		if (startsWith(line, "START"))
		{
			START(line);
			return;
		}
		if (startsWith(line, "RECTSTART"))
		{
			RECTSTART(line);
			return;
		}
		if (startsWith(line, "RESTART"))
		{
			RESTART();
			return;
		}
		if (startsWith(line, "SWAP2BOARD"))
		{
			SWAP2BOARD();
			return;
		}
		if (startsWith(line, "BEGIN"))
		{
			BEGIN();
			return;
		}
		if (startsWith(line, "BOARD"))
		{
			BOARD();
			return;
		}
		if (startsWith(line, "TURN"))
		{
			TURN(line);
			return;
		}
		if (startsWith(line, "TAKEBACK"))
		{
			TAKEBACK(line);
			return;
		}
		if (startsWith(line, "END"))
		{
			END();
			return;
		}
		if (startsWith(line, "PLAY"))
		{
			PLAY(line);
			return;
		}
		if (startsWith(line, "ABOUT"))
		{
			ABOUT();
			return;
		}
		if (startsWith(line, "YXSHOWINFO"))
		{
			YXSHOWINFO();
			return;
		}
		UNKNOWN(line);
	}
	void GomocupPlayer::respondWith(const std::string &answer)
	{
		log_line("Answered: " + answer);
		print_line(answer);
	}

	void GomocupPlayer::makeMove(Move move)
	{
		assert(move.row >= 0 && move.row < getBoard().rows());
		assert(move.col >= 0 && move.col < getBoard().cols());
		assert(move.sign == sign_to_move);
		assert(getBoard().at(move.row, move.col) == Sign::NONE);

		getBoard().at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(sign_to_move);
	}
	GomocupPlayerConfig GomocupPlayer::getConfig() const noexcept
	{
		return config;
	}
	SearchEngine& GomocupPlayer::getSearchEngine()
	{
		if (search_engine == nullptr)
		{
			if (config.game_config.rows != config.game_config.cols)
				ERROR("board is not square");
			if (config.game_config.rules != GameRules::FREESTYLE and config.game_config.rules != GameRules::STANDARD)
				ERROR("only freestyle and standard rules are supported");
			if (config.game_config.rules == GameRules::FREESTYLE and config.game_config.rows != 20)
				ERROR("freestyle rules is supported only on 20x20 board");
			if (config.game_config.rules == GameRules::STANDARD and config.game_config.rows != 15)
				ERROR("standard rules is supported only on 15x15 board");

			Json cfg = loadConfig();
//			search_engine = std::make_unique<SearchEngine>(cfg, config.game_config, *this);
		}
		return *search_engine;
	}
	matrix<Sign>& GomocupPlayer::getBoard()
	{
		if (board.size() == 0 and sign_to_move == Sign::NONE) // board is uninitialized
		{
			board = matrix<Sign>(config.game_config.rows, config.game_config.cols);
			sign_to_move = Sign::CROSS;
		}
		return board;
	}

	void GomocupPlayer::INFO(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		assert(tmp.size() == 3u);
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
		assert(tmp.size() == 2u);
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
		last_time = getTime();
		respondWith("OK");
	}
	void GomocupPlayer::SWAP2BOARD()
	{
		std::string line1 = get_line();
		if (line1 != "DONE") // place first three stones
		{
			std::string line2 = get_line();
			std::string line3 = get_line();
			makeMove(moveFromString(line1, Sign::CROSS));
			makeMove(moveFromString(line2, Sign::CIRCLE));
			makeMove(moveFromString(line3, Sign::CROSS));

			std::string line4 = get_line();
			if (line4 != "DONE")
			{
				// 5 stones were placed
				std::string line5 = get_line();
				std::string line6 = get_line(); // DONE
				assert(line6 == "DONE");
				makeMove(moveFromString(line4, Sign::CROSS));
				makeMove(moveFromString(line5, Sign::CIRCLE));
			}
		}
		getSearchEngine().swap2(board, sign_to_move);
	}
	void GomocupPlayer::BEGIN()
	{
		getBoard().clear();
		sign_to_move = Sign::CROSS;
		getSearchEngine().makeMove(board, sign_to_move);
	}
	void GomocupPlayer::BOARD()
	{
		getBoard().clear();
		sign_to_move = Sign::NONE;
		while (true)
		{
			std::string line = get_line();
			if (line == "DONE")
				break;
			auto tmp = split(line, ',');
			assert(tmp.size() == 3u);
			int row = std::stoi(tmp[1]);
			int col = std::stoi(tmp[0]);
			int sign = std::stoi(tmp[2]);
			if (sign_to_move == Sign::NONE)
			{
				if (sign == 1) //own stone
					sign_to_move = Sign::CROSS;
				else
					sign_to_move = Sign::CIRCLE;
			}
			getBoard().at(row, col) = (sign == 1) ? sign_to_move : invertSign(sign_to_move);
		}

		getSearchEngine().makeMove(board, sign_to_move);
	}
	void GomocupPlayer::TURN(const std::string &msg)
	{
		if (isBoardEmpty(getBoard()))
		{
			getBoard().clear();
			sign_to_move = Sign::CROSS;
		}
		auto tmp = split(msg, ' ');
		assert(tmp.size() == 2u);
		Move move = moveFromString(tmp[1], sign_to_move);
		std::cout << "got " << move.toString() << '\n';
		makeMove(move);
		getSearchEngine().makeMove(board, sign_to_move);
	}
	void GomocupPlayer::TAKEBACK(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		assert(tmp.size() == 2u);
		Move move = moveFromString(tmp[1], Sign::NONE);

		assert(getBoard().at(move.row, move.col) != Sign::NONE);
		getBoard().at(move.row, move.col) = Sign::NONE;

		getSearchEngine().stop();
		respondWith("OK");
	}
	void GomocupPlayer::END()
	{
		if(search_engine != nullptr) // if search_engine has not been created there's no need to do it here
			search_engine->stop();
		is_running = false;
	}

	void GomocupPlayer::SUGGEST()
	{
		ERROR("SUGGEST not supported");
	}
	void GomocupPlayer::PLAY(const std::string &msg)
	{
		auto tmp = split(msg, ' ');
		assert(tmp.size() == 2u);
		Move move = moveFromString(tmp[1], sign_to_move);
		getSearchEngine().stop();
		makeMove(move);
	}

	void GomocupPlayer::ABOUT()
	{
		std::string result;
		result += "name=\"AlphaGomoku\", ";
		result += "version=\"5.0\", ";
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
		result["use_logging"] = false;
		result["use_pondering"] = false;
		result["networks"] = {	{"freestyle", "/networks/freestyle_10x128.bin"},
								{"standard", "/networks/standard_10x128.bin"},
								{"renju", ""},
								{"caro", ""}};
		result["threads"][0] = Json( { { "device", "CPU" } });
		result["search_options"] = SearchConfig::getDefault();
		result["tree_options"] = TreeConfig::getDefault();
		result["cache_options"] = CacheConfig::getDefault();
		FileSaver fs(local_launch_path + "config.json");
		fs.save(result, SerializedObject(), 2);
	}

	std::string GomocupPlayer::get_line()
	{
		std::string line;
		getline(input_stream, line);

		if (line[line.length() - 1] == '\r')
			return line.substr(0, line.length() - 1);
		else
			return line;
	}
	void GomocupPlayer::print_line(const std::string &line)
	{
		output_stream << line << '\n';
	}
	void GomocupPlayer::log_line(const std::string &line)
	{
		if (config.use_logging)
			logging_stream << line << '\n';
	}

} /* namespace ag */

