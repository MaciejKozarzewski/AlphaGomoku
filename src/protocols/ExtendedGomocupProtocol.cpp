/*
 * ExtendedExtendedGomocupProtocol.cpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/ExtendedGomocupProtocol.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/version.hpp>

#include <cassert>

namespace ag
{

	ExtendedGomocupProtocol::ExtendedGomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			GomocupProtocol(queueIN, queueOUT)
	{
		// @formatter:off
		registerOutputProcessor(MessageType::BEST_MOVE, [this](OutputSender &sender) { this->best_move(sender);});

		registerInputProcessor("info evaluate",		[this](InputListener &listener) { this->info_evaluate(listener);});
		registerInputProcessor("info rule",			[this](InputListener &listener) { this->info_rule(listener);});
		registerInputProcessor("info analysis_mode",[this](InputListener &listener) { this->info_analysis_mode(listener);});
		registerInputProcessor("info max_depth", 	[this](InputListener &listener) { this->info_max_depth(listener);});
		registerInputProcessor("info max_node",		[this](InputListener &listener) { this->info_max_node(listener);});
		registerInputProcessor("info time_increment",[this](InputListener &listener) { this->info_time_increment(listener);});
		registerInputProcessor("info style",		[this](InputListener &listener) { this->info_style(listener);});
		registerInputProcessor("info auto_pondering",[this](InputListener &listener) { this->info_auto_pondering(listener);});
		registerInputProcessor("info protocol_lag",	[this](InputListener &listener) { this->info_protocol_lag(listener);});
		registerInputProcessor("info thread_num",	[this](InputListener &listener) { this->info_thread_num(listener);});

		registerInputProcessor("play",			  [this](InputListener &listener) { this->play(listener);});
		registerInputProcessor("turn",			  [this](InputListener &listener) { this->turn(listener);});
		registerInputProcessor("takeback",		  [this](InputListener &listener) { this->takeback(listener);});
		registerInputProcessor("ponder",		  [this](InputListener &listener) { this->ponder(listener);});
		registerInputProcessor("stop",			  [this](InputListener &listener) { this->stop(listener);});
		registerInputProcessor("showforbid",	  [this](InputListener &listener) { this->showforbid(listener);});
		registerInputProcessor("balance",		  [this](InputListener &listener) { this->balance(listener);});
		registerInputProcessor("clearhash",		  [this](InputListener &listener) { this->clearhash(listener);});
		registerInputProcessor("protocolversion", [this](InputListener &listener) { this->protocolversion(listener);});
		// openings
		registerInputProcessor("proboard",	   [this](InputListener &listener) { this->proboard(listener);});
		registerInputProcessor("longproboard", [this](InputListener &listener) { this->longproboard(listener);});
		registerInputProcessor("swapboard",	   [this](InputListener &listener) { this->swapboard(listener);});
		registerInputProcessor("swap2board",   [this](InputListener &listener) { this->swap2board(listener);});
// @formatter:on
	}
	void ExtendedGomocupProtocol::reset()
	{
		GomocupProtocol::reset();
	}
	ProtocolType ExtendedGomocupProtocol::getType() const noexcept
	{
		return ProtocolType::EXTENDED_GOMOCUP;
	}
	/*
	 * private
	 */
	/*
	 * Output processors
	 */
	void ExtendedGomocupProtocol::best_move(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		if (msg.holdsString()) // used to swap colors
		{
			if (msg.getString() == "swap")
				sender.send("SWAP");
		}
		if (msg.holdsMove()) // used to return the best move
		{
			assert(msg.getMove().sign == get_sign_to_move());
			if (is_in_analysis_mode)
			{
				sender.send("SUGGEST " + moveToString(msg.getMove()));
				expects_play_command = true;
			}
			else
			{
				sender.send(moveToString(msg.getMove()));
				list_of_moves.push_back(msg.getMove());
			}
		}
		if (msg.holdsListOfMoves()) // used to return multiple moves (for example an opening)
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
	}
	/*
	 * Input processors
	 */
	void ExtendedGomocupProtocol::info_evaluate(InputListener &listener)
	{
		const std::vector<std::string> tmp = split(extract_command_data(listener, "info evaluate"), ' ');
		std::vector<Move> path;
		for (size_t i = 0; i < tmp.size(); i++)
			path.push_back(moveFromString(tmp.at(i), Sign::NONE));
		input_queue.push(Message(MessageType::INFO_MESSAGE, path));
	}
	void ExtendedGomocupProtocol::info_rule(InputListener &listener)
	{
		const std::string data = extract_command_data(listener, "info rule");
		is_renju_rule = (std::stoi(data) == 4);
		switch (std::stoi(data))
		{
			case 0:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::FREESTYLE) }));
				break;
			case 1:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::STANDARD) }));
				break;
			case 2:
				output_queue.push(Message(MessageType::ERROR, "Continuous game is not supported"));
				break;
			case 4:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) }));
				break;
			case 8:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO6) }));
				break;
			case 9:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO5) }));
				break;
			default:
				output_queue.push(Message(MessageType::ERROR, "Invalid rule " + data));
				break;
		}
	}
	void ExtendedGomocupProtocol::info_analysis_mode(InputListener &listener)
	{
		std::string data = extract_command_data(listener, "info analysis_mode");
		is_in_analysis_mode = (std::stoi(data) == 1);
		input_queue.push(Message(MessageType::SET_OPTION, Option { "analysis_mode", data }));
	}
	void ExtendedGomocupProtocol::info_max_depth(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_depth", extract_command_data(listener, "info max_depth") }));
	}
	void ExtendedGomocupProtocol::info_max_node(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_node", extract_command_data(listener, "info max_node") }));
	}
	void ExtendedGomocupProtocol::info_time_increment(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "time_increment", extract_command_data(listener, "info time_increment") }));
	}
	void ExtendedGomocupProtocol::info_style(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "style", extract_command_data(listener, "info style") }));
	}
	void ExtendedGomocupProtocol::info_auto_pondering(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "auto_pondering", extract_command_data(listener, "info auto_pondering") }));
	}
	void ExtendedGomocupProtocol::info_protocol_lag(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "protocol_lag", extract_command_data(listener, "info protocol_lag") }));
	}
	void ExtendedGomocupProtocol::info_thread_num(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "thread_num", extract_command_data(listener, "info thread_num") }));
	}
	void ExtendedGomocupProtocol::play(InputListener &listener)
	{
		std::string line = listener.getLine();
		if (not expects_play_command)
			throw ProtocolRuntimeException("Was not expecting PLAY command");

		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const Move new_move = moveFromString(tmp.at(1), get_sign_to_move());
		add_new_move(new_move);
		expects_play_command = false;
		output_queue.push(Message(MessageType::PLAIN_STRING, tmp.at(1)));
	}
	void ExtendedGomocupProtocol::turn(InputListener &listener)
	{
		if (expects_play_command)
		{
			listener.consumeLine();
			throw ProtocolRuntimeException("Expecting PLAY command");
		}
		else
			GomocupProtocol::turn(listener);
	}
	void ExtendedGomocupProtocol::takeback(InputListener &listener)
	{
		if (expects_play_command)
		{
			listener.consumeLine();
			throw ProtocolRuntimeException("Expecting PLAY command");
		}
		else
			GomocupProtocol::takeback(listener);
	}

	void ExtendedGomocupProtocol::ponder(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() == 1)
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", "-1" }));
		else
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_pondering", tmp.at(1) }));

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "ponder"));
	}
	void ExtendedGomocupProtocol::stop(InputListener &listener)
	{
		listener.consumeLine("stop");
		input_queue.push(Message(MessageType::STOP_SEARCH));
	}
	void ExtendedGomocupProtocol::showforbid(InputListener &listener)
	{
		listener.consumeLine("showforbid");
		std::vector<Move> moves = parse_list_of_moves(listener, "DONE");
		if (is_renju_rule)
		{
			const matrix<Sign> board = Board::fromListOfMoves(rows, columns, moves);
			std::string response = "FORBID";
			for (int r = 0; r < board.rows(); r++)
				for (int c = 0; c < board.cols(); c++)
					if (isForbidden(board, Move(r, c, Sign::CROSS)))
						response += " " + moveToString(Move(r, c));
			output_queue.push(Message(MessageType::PLAIN_STRING, response));
		}
		else
			output_queue.push(Message(MessageType::PLAIN_STRING, "FORBID"));
	}
	void ExtendedGomocupProtocol::balance(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::ERROR, "Command not supported")); // TODO add position balancing

//		std::vector<std::string> tmp = split(line, ' ');
//		if (tmp.size() != 2u)
//			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");
//
//		list_of_moves = parse_list_of_moves(listener, "DONE");
//
//		input_queue.push(Message(MessageType::STOP_SEARCH));
//		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
//		input_queue.push(Message(MessageType::START_SEARCH, "balance " + tmp.at(1)));
	}
	void ExtendedGomocupProtocol::clearhash(InputListener &listener)
	{
		listener.consumeLine("clearhash");
		std::vector<Move> tmp;
		for (int r = 0; r < rows; r++)
			for (int c = 0; c < columns; c++)
				tmp.push_back(Move(Sign::CIRCLE, r, c)); // creating impossible board with all circles (could also be all crosses)

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, tmp)); // Node cache deletes states that are provably not possible given current board state. Given an impossible board state, all nodes will be cleared.
		input_queue.push(Message(MessageType::START_SEARCH, "clearhash")); // launching search with special controller that will pass this board to the engine causing tree reset but without actually starting the search.
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves)); // reverting to the original board state
	}
	void ExtendedGomocupProtocol::protocolversion(InputListener &listener)
	{
		listener.consumeLine("protocolversion");
		output_queue.push(Message(MessageType::PLAIN_STRING, "1,0"));
	}
	/*
	 * opening rules
	 */
	void ExtendedGomocupProtocol::proboard(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void ExtendedGomocupProtocol::longproboard(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
	void ExtendedGomocupProtocol::swapboard(InputListener &listener)
	{
		listener.consumeLine("swapboard");
		list_of_moves = parse_ordered_moves(listener, "done");

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "swap"));
	}
	void ExtendedGomocupProtocol::swap2board(InputListener &listener)
	{
		listener.consumeLine("swap2board");
		list_of_moves = parse_ordered_moves(listener, "done");

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "swap2"));
	}
} /* namespace ag */

