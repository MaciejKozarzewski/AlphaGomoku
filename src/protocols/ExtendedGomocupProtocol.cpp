/*
 * ExtendedExtendedGomocupProtocol.cpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/ExtendedGomocupProtocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/version.hpp>

#include <libml/hardware/Device.hpp>

#include <cassert>

namespace ag
{

	ExtendedGomocupProtocol::ExtendedGomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			GomocupProtocol(queueIN, queueOUT)
	{
	}
	void ExtendedGomocupProtocol::reset()
	{
		GomocupProtocol::reset();
	}
	ProtocolType ExtendedGomocupProtocol::getType() const noexcept
	{
		return ProtocolType::EXTENDED_GOMOCUP;
	}
	void ExtendedGomocupProtocol::processInput(InputListener &listener)
	{
		std::string line = listener.peekLine();

		std::lock_guard lock(protocol_mutex);

		if (startsWith(line, "PLAY"))
		{
			PLAY(listener);
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
		if (startsWith(line, "SHOWFORBID"))
		{
			SHOWFORBID(listener);
			return;
		}
		if (startsWith(line, "BALANCE"))
		{
			BALANCE(listener);
			return;
		}
		if (startsWith(line, "CLEARHASH"))
		{
			CLEARHASH(listener);
			return;
		}
		if (startsWith(line, "PROTOCOLVERSION"))
		{
			PROTOCOLVERSION(listener);
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

		if (startsWith(line, "START"))
		{
			this->START(listener);
			return;
		}
		if (startsWith(line, "RECTSTART"))
		{
			this->RECTSTART(listener);
			return;
		}

		if (startsWith(line, "INFO"))
			this->INFO(listener); // intentionally does not have a return as other INFO keys must be processed by base class

		if (listener.isEmpty() == false) // call the base class only if there is anything to process, otherwise it will block waiting for input
			GomocupProtocol::processInput(listener);
	}
	void ExtendedGomocupProtocol::processOutput(OutputSender &sender)
	{
		std::lock_guard lock(protocol_mutex);

		while (output_queue.isEmpty() == false)
		{
			Message msg = output_queue.pop();
			switch (msg.getType())
			{
				case MessageType::BEST_MOVE:
				{
					if (msg.holdsString()) // used to swap colors
					{
						if (msg.getString() == "swap")
							sender.send("SWAP");
					}
					if (msg.holdsMove()) // used to return the best move
					{
						assert(msg.getMove().sign == get_sign_to_move());
						if (is_in_analysis_mode)
							sender.send("SUGGEST " + moveToString(msg.getMove()));
						else
						{
							sender.send(moveToString(msg.getMove()));
							list_of_moves.push_back(msg.getMove());
						}
					}
					if (msg.holdsListOfMoves()) // used to return an opening
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
					sender.send("UNKNOWN '" + msg.getString() + "'");
					break;
				case MessageType::ERROR:
					sender.send("ERROR " + msg.getString());
					break;
				case MessageType::INFO_MESSAGE:
				{
					if (msg.holdsString())
						sender.send("MESSAGE " + msg.getString());
					if (msg.holdsSearchSummary())
						sender.send("MESSAGE " + parse_search_summary(msg.getSearchSummary()));
					break;
				}
				case MessageType::ABOUT_ENGINE:
					sender.send(msg.getString());
					break;
				default:
					break;
			}
		}
	}
	/*
	 * private
	 */
	void ExtendedGomocupProtocol::START(InputListener &listener)
	{
		std::string line = listener.peekLine();
		std::vector<std::string> tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		rows = std::stoi(tmp[1]);
		columns = std::stoi(tmp[1]);

		GomocupProtocol::START(listener);
	}
	void ExtendedGomocupProtocol::RECTSTART(InputListener &listener)
	{
		std::string line = listener.peekLine();
		std::vector<std::string> tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		tmp = split(tmp[1], ',');
		assert(tmp.size() == 2u);
		rows = std::stoi(tmp[1]);
		columns = std::stoi(tmp[0]);

		GomocupProtocol::RECTSTART(listener);
	}
	void ExtendedGomocupProtocol::INFO(InputListener &listener)
	{
		std::string line = listener.peekLine();
		std::vector<std::string> tmp = split(line, ' ');
		assert(tmp.size() >= 3u);
		if (tmp[1] == "evaluate")
		{
			std::vector<Move> path;
			for (size_t i = 2; i < tmp.size(); i++)
				path.push_back(moveFromString(tmp[i], Sign::NONE));
			input_queue.push(Message(MessageType::INFO_MESSAGE, path));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "analysis_mode")
		{
			is_in_analysis_mode = static_cast<bool>(std::stoi(tmp[2]));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "max_depth")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "max_depth", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "max_node")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "max_nodes", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "time_increment")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_increment", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "style")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "style", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "auto_pondering")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "auto_pondering", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "protocol_lag")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "protocol_lag", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "thread_num")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "thread_num", tmp[2] }));
			listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
			return;
		}
		if (tmp[1] == "rule")
		{
			switch (std::stoi(tmp[2]))
			{
				case 4:
					is_renju_rule = true;
					return;
				case 8:
					is_renju_rule = false;
//					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO) })); TODO uncomment this one this rule is supported
					output_queue.push(Message(MessageType::ERROR, "caro rule is not supported"));
					listener.consumeLine(); // consuming line so it won't be processed again by base GomocupProtocol class
					return;
				default:
					is_renju_rule = false;
					return;
			}
		}
	}

	void ExtendedGomocupProtocol::PLAY(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		Move new_move = moveFromString(tmp[1], get_sign_to_move());
		if (std::find(list_of_moves.begin(), list_of_moves.end(), new_move) == list_of_moves.end()) // new move must not be in the list
		{
			list_of_moves.push_back(new_move);
			output_queue.push(Message(MessageType::BEST_MOVE, new_move));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Invalid move '" + tmp[1] + "'"));
	}
	void ExtendedGomocupProtocol::PONDER(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() == 1)
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", "-1" }));
		else
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", tmp[1] }));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "ponder"));
	}
	void ExtendedGomocupProtocol::STOP(InputListener &listener)
	{
		listener.consumeLine("STOP");
		input_queue.push(Message(MessageType::STOP_SEARCH));
	}
	void ExtendedGomocupProtocol::SHOWFORBID(InputListener &listener)
	{
		listener.consumeLine("SHOWFORBID");
		std::vector<Move> moves = parse_list_of_moves(listener, "DONE");
		if (is_renju_rule)
		{
			// TODO add detecting of forbidden moves for renju
		}
		else
			output_queue.push(Message(MessageType::PLAIN_STRING, "FORBID"));
	}
	void ExtendedGomocupProtocol::BALANCE(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		assert(tmp.size() == 2u);
		list_of_moves = parse_list_of_moves(listener, "DONE");
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "balance " + tmp[1]));
	}
	void ExtendedGomocupProtocol::CLEARHASH(InputListener &listener)
	{
		listener.consumeLine("CLEARHASH");
		std::vector<Move> tmp;
		for (int r = 0; r < rows; r++)
			for (int c = 0; c < columns; c++)
				tmp.push_back(Move(Sign::CIRCLE, r, c)); // creating impossible board with all circles (could also be all crosses)

		input_queue.push(Message(MessageType::SET_POSITION, tmp)); // Node cache deletes states that are provably not possible given current board state. Given an impossible board state, all nodes will be cleared.
		input_queue.push(Message(MessageType::START_SEARCH, "clearhash")); // launching search with special controller that will pass this board to the engine, causing tree reset.
	}
	void ExtendedGomocupProtocol::PROTOCOLVERSION(InputListener &listener)
	{
		listener.consumeLine("PROTOCOLVERSION");
		output_queue.push(Message(MessageType::PLAIN_STRING, "1,0"));
	}

	// opening rules
	void ExtendedGomocupProtocol::PROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void ExtendedGomocupProtocol::LONGPROBOARD(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void ExtendedGomocupProtocol::SWAPBOARD(InputListener &listener)
	{
		listener.consumeLine("SWAPBOARD");

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
			if (line4 != "DONE")
			{
				output_queue.push(Message(MessageType::ERROR, "incorrect SWAPBOARD command"));
				return;
			}
		}

		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "swap"));
	}
	void ExtendedGomocupProtocol::SWAP2BOARD(InputListener &listener)
	{
		listener.consumeLine("SWAP2BOARD");

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
				list_of_moves.push_back(moveFromString(line4, Sign::CIRCLE));
				list_of_moves.push_back(moveFromString(line5, Sign::CROSS));

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
} /* namespace ag */

