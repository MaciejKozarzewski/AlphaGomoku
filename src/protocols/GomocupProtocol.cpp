/*
 * GomocupProtocol.cpp
 *
 *  Created on: 5 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <libml/hardware/Device.hpp>

namespace ag
{

	GomocupProtocol::GomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			Protocol(queueIN, queueOUT)
	{
		queueOUT.push(Message(MessageType::INFO_MESSAGE, "Detected following devices"));
		queueOUT.push(Message(MessageType::INFO_MESSAGE, ml::Device::cpu().toString() + " : " + ml::Device::cpu().info()));
		for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
			queueOUT.push(Message(MessageType::INFO_MESSAGE, ml::Device::cuda(i).toString() + " : " + ml::Device::cuda(i).info()));
	}
	ProtocolType GomocupProtocol::getType() const noexcept
	{
		return ProtocolType::GOMOCUP;
	}
	void GomocupProtocol::processInput(InputListener &listener)
	{
		std::string line = listener.peekLine();

		if (startsWith(line, "INFO"))
		{
			INFO(listener);
			return;
		}

		if (startsWith(line, "START"))
		{
			START(listener);
			return;
		}
		if (startsWith(line, "RECTSTART"))
		{
			RECTSTART(listener);
			return;
		}
		if (startsWith(line, "RESTART"))
		{
			RESTART(listener);
			return;
		}

		if (startsWith(line, "PROBOARD"))
		{
			PROBOARD(listener);
			return;
		}
		if (startsWith(line, "LONGPROBOARD"))
		{
			LONGPROBOARD(listener);
			return;
		}
		if (startsWith(line, "SWAPBOARD"))
		{
			SWAPBOARD(listener);
			return;
		}
		if (startsWith(line, "SWAP2BOARD"))
		{
			SWAP2BOARD(listener);
			return;
		}

		if (startsWith(line, "BEGIN"))
		{
			BEGIN(listener);
			return;
		}
		if (startsWith(line, "BOARD"))
		{
			BOARD(listener);
			return;
		}
		if (startsWith(line, "TURN"))
		{
			TURN(listener);
			return;
		}
		if (startsWith(line, "PONDER"))
		{
			PONDER(listener);
			return;
		}
		if (startsWith(line, "STOP"))
		{
			STOP(listener);
			return;
		}
		if (startsWith(line, "TAKEBACK"))
		{
			TAKEBACK(listener);
			return;
		}
		if (startsWith(line, "END"))
		{
			END(listener);
			return;
		}

		if (startsWith(line, "ABOUT"))
		{
			ABOUT(listener);
			return;
		}

		UNKNOWN(listener);
	}
	void GomocupProtocol::processOutput(OutputSender &sender)
	{
		while (output_queue.isEmpty() == false)
		{
			Message msg = output_queue.pop();
			switch (msg.getType())
			{
				case MessageType::MAKE_MOVE:
				{
					if (msg.holdsString()) // used to swap colors
					{
						if (msg.getString() == "swap")
							sender.send("SWAP");
					}
					if (msg.holdsMove()) // used to return best move
					{
						assert(msg.getMove().sign == get_sign_to_move());
						sender.send(moveToString(msg.getMove()));
						std::lock_guard lock(board_mutex);
						list_of_moves.push_back(msg.getMove());
					}
					if (msg.holdsListOfMoves()) // used in swap2
					{
						std::string str;
						for (size_t i = 0; i < msg.getListOfMoves().size(); i++)
						{
							if (i != 0)
								str += ' ';
							str += moveToString(msg.getListOfMoves().at(i));
						}
						sender.send(str);
					}
					break;
				}
				case MessageType::PLAIN_STRING:
					sender.send(msg.getString());
					break;
				case MessageType::UNKNOWN_COMMAND:
					sender.send("UNKNOWN " + msg.getString());
					break;
				case MessageType::ERROR:
					sender.send("ERROR " + msg.getString());
					break;
				case MessageType::INFO_MESSAGE:
					sender.send("MESSAGE " + msg.getString());
					break;
				case MessageType::ABOUT_ENGINE:
					sender.send(msg.getString());
					break;
				default:
					break;
			}
		}
	}

// private
	Sign GomocupProtocol::get_sign_to_move() const noexcept
	{
		std::lock_guard lock(board_mutex);
		if (list_of_moves.empty())
			return Sign::CROSS;
		else
			return invertSign(list_of_moves.back().sign);
	}
	void GomocupProtocol::INFO(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 3u);
		if (tmp[1] == "timeout_turn")
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_turn", tmp[2] }));
		if (tmp[1] == "timeout_match")
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_match", tmp[2] }));
		if (tmp[1] == "time_left")
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_left", tmp[2] }));
		if (tmp[1] == "max_memory")
			input_queue.push(Message(MessageType::SET_OPTION, Option { "max_memory", tmp[2] }));
		if (tmp[1] == "game_type")
			input_queue.push(Message());
		if (tmp[1] == "rule")
		{
			switch (std::stoi(tmp[2]))
			{
				case 0:
					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::FREESTYLE) }));
					break;
				case 1:
					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::STANDARD) }));
					break;
				case 2:
					input_queue.push(Message());
					output_queue.push(Message(MessageType::ERROR, "continuous game is not supported"));
					break;
				case 4:
					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) }));
					output_queue.push(Message(MessageType::ERROR, "renju rule is not supported"));
					break;
				case 8:
					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO) }));
					output_queue.push(Message(MessageType::ERROR, "caro rule is not supported"));
					break;
			}
		}
		if (tmp[1] == "evaluate")
			input_queue.push(Message());
		if (tmp[1] == "folder")
			input_queue.push(Message(MessageType::SET_OPTION, Option { "folder", tmp[2] }));
	}
	void GomocupProtocol::START(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		input_queue.push(Message(MessageType::START_PROGRAM));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", tmp[1] }));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "cols", tmp[1] }));
		if (std::stoi(tmp[1]) == 15 or std::stoi(tmp[1]) == 20)
			output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
		else
			output_queue.push(Message(MessageType::ERROR, "only 15x15 or 20x20 boards are supported"));
	}
	void GomocupProtocol::RECTSTART(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		tmp = split(tmp[1], ',');
		assert(tmp.size() == 2u);
//		input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", tmp[1] }));
//		input_queue.push(Message(MessageType::SET_OPTION, Option { "cols", tmp[0] }));
//		input_queue.push(Message(MessageType::START_PROGRAM));
		output_queue.push(Message(MessageType::ERROR, "rectangular boards are not supported"));
	}
	void GomocupProtocol::RESTART(InputListener &listener)
	{
		listener.getLine(); // consuming 'RESTART' line
		input_queue.push(Message(MessageType::START_PROGRAM));
		output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
	}
	void GomocupProtocol::PROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void GomocupProtocol::LONGPROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void GomocupProtocol::SWAPBOARD(InputListener &listener)
	{
		listener.getLine(); // consuming 'SWAPBOARD' line
		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
			list_of_moves.clear();
		}

		std::string line1 = listener.getLine();
		if (line1 != "DONE") // 3 stones were placed
		{
			std::string line2 = listener.getLine();
			std::string line3 = listener.getLine();
			{ 	// artificial scope for lock
				std::lock_guard lock(board_mutex);
				list_of_moves.push_back(moveFromString(line1, Sign::CROSS));
				list_of_moves.push_back(moveFromString(line2, Sign::CIRCLE));
				list_of_moves.push_back(moveFromString(line3, Sign::CROSS));
			}

			std::string line4 = listener.getLine();
			if (line4 != "DONE")
			{
				output_queue.push(Message(MessageType::ERROR, "incorrect SWAPBOARD command"));
				return;
			}
		}

		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "swap"));
	}
	void GomocupProtocol::SWAP2BOARD(InputListener &listener)
	{
		listener.getLine(); // consuming 'SWAP2BOARD' line
		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
			list_of_moves.clear();
		}

		std::string line1 = listener.getLine();
		if (line1 != "DONE") // 3 stones were placed
		{
			std::string line2 = listener.getLine();
			std::string line3 = listener.getLine();
			{ 	// artificial scope for lock
				std::lock_guard lock(board_mutex);
				list_of_moves.push_back(moveFromString(line1, Sign::CROSS));
				list_of_moves.push_back(moveFromString(line2, Sign::CIRCLE));
				list_of_moves.push_back(moveFromString(line3, Sign::CROSS));
			}

			std::string line4 = listener.getLine();
			if (line4 != "DONE") // 5 stones were placed
			{
				std::string line5 = listener.getLine();
				std::string line6 = listener.getLine(); // DONE
				{ 	// artificial scope for lock
					std::lock_guard lock(board_mutex);
					list_of_moves.push_back(moveFromString(line4, Sign::CIRCLE));
					list_of_moves.push_back(moveFromString(line5, Sign::CROSS));
				}

				if (line6 != "DONE")
				{
					output_queue.push(Message(MessageType::ERROR, "incorrect SWAP2BOARD command"));
					return;
				}
			}
		}
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "swap2"));
	}
	void GomocupProtocol::BEGIN(InputListener &listener)
	{
		listener.getLine(); // consuming 'BEGIN' line
		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
			list_of_moves.clear();
		}
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::BOARD(InputListener &listener)
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
			{
				output_queue.push(Message(MessageType::ERROR, "incorrectly specified board"));
				return; // impossible situation, incorrectly specified board state
			}
		}

		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
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
		}
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::TURN(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		Move m = moveFromString(tmp[1], get_sign_to_move());
		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
			list_of_moves.push_back(m);
		}
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::PONDER(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		if (tmp.size() == 1)
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", "-1" }));
		else
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", tmp[1] }));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "ponder"));
	}
	void GomocupProtocol::STOP(InputListener &listener)
	{
		listener.getLine(); // consuming 'STOP' line
		input_queue.push(Message(MessageType::STOP_SEARCH));
	}
	void GomocupProtocol::TAKEBACK(InputListener &listener)
	{
		std::string line = listener.getLine();
		auto tmp = split(line, ' ');
		assert(tmp.size() == 2u);

		int number_of_moves;
		Move last_move;
		{ 	// artificial scope for lock
			std::lock_guard lock(board_mutex);
			number_of_moves = list_of_moves.size();
			last_move = list_of_moves.back();
		}

		if (number_of_moves == 0)
			output_queue.push(Message(MessageType::ERROR, "the board is empty"));
		else
		{
			Move m = moveFromString(tmp[1], last_move.sign);
			if (last_move != m)
				output_queue.push(Message(MessageType::ERROR, "can undo only last move"));
			else
			{
				{ 	// artificial scope for lock
					std::lock_guard lock(board_mutex);
					list_of_moves.pop_back();
				}
				input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
				output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
			}
		}
	}
	void GomocupProtocol::END(InputListener &listener)
	{
		listener.getLine(); // consuming 'END' line
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::EXIT_PROGRAM));
	}
	void GomocupProtocol::ABOUT(InputListener &listener)
	{
		listener.getLine(); // consuming 'ABOUT' line
		std::string result;
		result += "name=\"AlphaGomoku\", ";
		result += "version=\"5.0\", ";
		result += "author=\"Maciej Kozarzewski\", ";
		result += "country=\"Poland\", ";
		result += "www=\"https://github.com/MaciejKozarzewski/AlphaGomoku\", ";
		result += "email=\"alphagomoku.mk@gmail.com\"";
		output_queue.push(Message(MessageType::ABOUT_ENGINE, result));
	}
	void GomocupProtocol::UNKNOWN(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
} /* namespace ag */

