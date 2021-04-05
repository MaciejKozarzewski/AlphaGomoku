/*
 * GomocupProtocol.cpp
 *
 *  Created on: 5 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{

	ProtocolType GomocupProtocol::getType() const noexcept
	{
		return ProtocolType::GOMOCUP;
	}
	Message GomocupProtocol::processInput(InputListener &listener)
	{
		std::string line = listener.peekLine();
		Logger::write("GomocupProtocol got: " + line);

		if (startsWith(line, "INFO"))
			return INFO(listener);

		if (startsWith(line, "START"))
			return START(listener);
		if (startsWith(line, "RECTSTART"))
			return RECTSTART(listener);
		if (startsWith(line, "RESTART"))
			return RESTART(listener);

		if (startsWith(line, "PROBOARD"))
			return PROBOARD(listener);
		if (startsWith(line, "LONGPROBOARD"))
			return LONGPROBOARD(listener);
		if (startsWith(line, "SWAPBOARD"))
			return SWAPBOARD(listener);
		if (startsWith(line, "SWAP2BOARD"))
			return SWAP2BOARD(listener);

		if (startsWith(line, "BEGIN"))
			return BEGIN(listener);
		if (startsWith(line, "BOARD"))
			return BOARD(listener);
		if (startsWith(line, "TURN"))
			return TURN(listener);
		if (startsWith(line, "TAKEBACK"))
			return TAKEBACK(listener);
		if (startsWith(line, "END"))
			return END(listener);

		if (startsWith(line, "PLAY"))
			return PLAY(listener);
		if (startsWith(line, "SUGGEST"))
			return SUGGEST(listener);

		if (startsWith(line, "ABOUT"))
			return ABOUT(listener);

		return UNKNOWN(listener);
	}
	bool GomocupProtocol::processOutput(const Message &msg, OutputSender &sender)
	{
		switch (msg.getType())
		{
			case MessageType::CHANGE_PROTOCOL:
				return true; // does not require response
			case MessageType::START_PROGRAM:
				return false; // does not require response, but should never occur
			case MessageType::SET_OPTION:
				return false; // does not require response, but should never occur
			case MessageType::SET_POSITION:
				return false; // does not require response, but should never occur
			case MessageType::MAKE_MOVE:
			{
				assert(msg.getMove().sign == get_sign_to_move());
				list_of_moves.push_back(msg.getMove());
				sender.send(moveToString(msg.getMove()));
				return true;
			}
			case MessageType::ABOUT_ENGINE:
				return false; // does not require response, but should never occur
			case MessageType::STOP_SEARCH:
				return true; // does not require response
			case MessageType::EXIT_PROGRAM:
				return true; // does not require response
			case MessageType::EMPTY_MESSAGE:
				return true; // does not require response
			case MessageType::PLAIN_STRING:
			{
				sender.send(msg.getString());
				return true;
			}
			case MessageType::UNKNOWN_MESSAGE:
			{
				sender.send("UNKNOWN " + msg.getString());
				return true;
			}
			case MessageType::ERROR:
			{
				sender.send("ERROR " + msg.getString());
				return true;
			}
			case MessageType::OPENING_PRO:
			{
				break;
			}
			case MessageType::OPENING_LONG_PRO:
			{
				break;
			}
			case MessageType::OPENING_SWAP:
			{
				break;
			}
			case MessageType::OPENING_SWAP2:
			{
				break;
			}
		}
		return false;
	}

	// private
	Sign GomocupProtocol::get_sign_to_move() const noexcept
	{
		return (list_of_moves.size() == 0) ? Sign::CROSS : invertSign(list_of_moves.back().sign);
	}
	Message GomocupProtocol::INFO(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 3u);
		if (tmp[1] == "timeout_turn")
			return Message(MessageType::SET_OPTION, Option { "time_for_turn", tmp[2] });
		if (tmp[1] == "timeout_match")
			return Message(MessageType::SET_OPTION, Option { "time_for_match", tmp[2] });
		if (tmp[1] == "time_left")
			return Message(MessageType::SET_OPTION, Option { "time_left", tmp[2] });
		if (tmp[1] == "max_memory")
			return Message(MessageType::SET_OPTION, Option { "max_memory", tmp[2] });
		if (tmp[1] == "game_type")
			return Message();
		if (tmp[1] == "rule")
		{
			switch (std::stoi(tmp[2]))
			{
				case 0:
					return Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::FREESTYLE) });
				case 1:
					return Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::STANDARD) });
				case 2:
					break; // continuous game - not supported
				case 4:
					return Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) });
				case 8:
					return Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO) });
			}
			return Message();
		}
		if (tmp[1] == "evaluate")
			return Message();
		if (tmp[1] == "folder")
			return Message(MessageType::SET_OPTION, Option { "folder", tmp[2] });

		return Message();
	}
	Message GomocupProtocol::START(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		int board_size = std::stoi(tmp[1]);
		return Message(MessageType::START_PROGRAM, GameConfig(board_size, board_size));
	}
	Message GomocupProtocol::RECTSTART(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		tmp = split(tmp[1], ',');
		assert(tmp.size() == 2u);
		int cols = std::stoi(tmp[0]);
		int rows = std::stoi(tmp[1]);
		return Message(MessageType::START_PROGRAM, GameConfig(rows, cols));
	}
	Message GomocupProtocol::RESTART(InputListener &listener)
	{
		listener.getLine(); // consuming 'RESTART' line
		return Message(MessageType::START_PROGRAM);
	}
	Message GomocupProtocol::PROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		return Message(MessageType::UNKNOWN_MESSAGE, line);
	}
	Message GomocupProtocol::LONGPROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		return Message(MessageType::UNKNOWN_MESSAGE, line);
	}
	Message GomocupProtocol::SWAPBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		return Message(MessageType::UNKNOWN_MESSAGE, line);
	}
	Message GomocupProtocol::SWAP2BOARD(InputListener &listener)
	{
		listener.getLine(); // consuming 'SWAP2BOARD' line
		list_of_moves.clear();

		std::string line1 = listener.getLine();
		if (line1 != "DONE") // 3 stones were placed
		{
			std::string line2 = listener.getLine();
			std::string line3 = listener.getLine();
			list_of_moves.push_back(moveFromString(line1, Sign::CROSS));
			list_of_moves.push_back(moveFromString(line2, Sign::CIRCLE));
			list_of_moves.push_back(moveFromString(line3, Sign::CROSS));

			std::string line4 = listener.getLine();
			if (line4 != "DONE") // 5 stones were placed
			{
				std::string line5 = listener.getLine();
				std::string line6 = listener.getLine(); // DONE
				assert(line6 == "DONE");
				list_of_moves.push_back(moveFromString(line4, Sign::CROSS));
				list_of_moves.push_back(moveFromString(line5, Sign::CIRCLE));
			}
		}
		return Message(MessageType::OPENING_SWAP2, list_of_moves);
	}
	Message GomocupProtocol::BEGIN(InputListener &listener)
	{
		listener.getLine(); // consuming 'BEGIN' line
		list_of_moves.clear();
		return Message(MessageType::SET_POSITION, list_of_moves);
	}
	Message GomocupProtocol::BOARD(InputListener &listener)
	{
		listener.getLine(); // consuming 'BOARD' line
		std::vector<Move> own_moves;
		std::vector<Move> opp_moves;
		while (true)
		{
			std::string line = listener.getLine();
			if (line == "DONE")
				break;
			auto tmp = split(line, ',');
			assert(tmp.size() == 3u);
			Move m(std::stoi(tmp[1]), std::stoi(tmp[0]));
			switch (std::stoi(tmp[2]))
			{
				case 1:
					own_moves.push_back(m);
					break;
				case 2:
					opp_moves.push_back(m);
					break;
				default:
				case 3: // continuous game, not supported
					break;
			}
		}

		list_of_moves.clear();
		if (own_moves.size() == opp_moves.size()) // I started the game as first player (CROSS)
		{
			std::for_each(own_moves.begin(), own_moves.end(), [](Move &m)
			{	m.sign = Sign::CROSS;});
			std::for_each(opp_moves.begin(), opp_moves.end(), [](Move &m)
			{	m.sign = Sign::CIRCLE;});
		}
		else
		{
			if (own_moves.size() + 1 == opp_moves.size()) // opponent started the game as first player (CROSS)
			{
				std::for_each(own_moves.begin(), own_moves.end(), [](Move &m)
				{	m.sign = Sign::CIRCLE;});
				std::for_each(opp_moves.begin(), opp_moves.end(), [](Move &m)
				{	m.sign = Sign::CROSS;});
			}
			else
				// impossible situation, incorrectly specified board state
				return Message(MessageType::ERROR, "incorrectly specified board");
		}

		list_of_moves.clear();
		if (own_moves.size() != opp_moves.size())
		{
			list_of_moves.push_back(opp_moves.front());
			opp_moves.erase(opp_moves.begin());
			assert(own_moves.size() == opp_moves.size());
		}
		for (size_t i = 0; i < own_moves.size(); i++)
		{
			list_of_moves.push_back(own_moves[i]);
			list_of_moves.push_back(opp_moves[i]);
		}
		return Message(MessageType::SET_POSITION, list_of_moves);
	}
	Message GomocupProtocol::TURN(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		Move m = moveFromString(tmp[1], get_sign_to_move());
		list_of_moves.push_back(m);
		return Message(MessageType::SET_POSITION, list_of_moves);
	}
	Message GomocupProtocol::TAKEBACK(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		if (list_of_moves.size() == 0)
			return Message(MessageType::ERROR, "the board is empty");
		else
		{
			Move m = moveFromString(tmp[1], list_of_moves.back().sign);
			if (list_of_moves.back() != m)
				return Message(MessageType::ERROR, "can undo only last move");
			else
			{
				list_of_moves.pop_back();
				return Message(MessageType::SET_POSITION, list_of_moves);
			}
		}
	}
	Message GomocupProtocol::END(InputListener &listener)
	{
		listener.getLine(); // consuming 'END' line
		return Message(MessageType::EXIT_PROGRAM);
	}
	Message GomocupProtocol::PLAY(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		Move m = moveFromString(tmp[1], get_sign_to_move());
		list_of_moves.push_back(m);
		return Message(MessageType::SET_POSITION, list_of_moves);
	}
	Message GomocupProtocol::ABOUT(InputListener &listener)
	{
		listener.getLine(); // consuming 'ABOUT' line
		return Message(MessageType::ABOUT_ENGINE);
	}
	Message GomocupProtocol::UNKNOWN(InputListener &listener)
	{
		std::string line = listener.getLine();
		return Message(MessageType::UNKNOWN_MESSAGE, line);
	}
} /* namespace ag */

