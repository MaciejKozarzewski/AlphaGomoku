/*
 * YixinBoardProtocol.cpp
 *
 *  Created on: Jun 13, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/YixinBoardProtocol.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/version.hpp>

#include <minml/core/Device.hpp>

#include <cassert>

namespace
{
	std::string move_yx_encoding(ag::Move m)
	{
		return std::to_string(m.col) + " " + std::to_string(m.row);
	}
	void consume_list_of_moves(ag::InputListener &listener, const std::string &ending)
	{
		while (true)
		{
			const std::string line = listener.getLine();
			if (line == ending)
				break;
		}
	}
}

namespace ag
{

	YixinBoardProtocol::YixinBoardProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			GomocupProtocol(queueIN, queueOUT)
	{
		// @formatter:off
		registerOutputProcessor(MessageType::BEST_MOVE, [this](OutputSender &sender) { this->best_move(sender);});

		registerInputProcessor("info max_depth", 	[this](InputListener &listener) { this->info_max_depth(listener);});
		registerInputProcessor("info max_node",		[this](InputListener &listener) { this->info_max_node(listener);});
		registerInputProcessor("info time_increment",[this](InputListener &listener) { this->info_time_increment(listener);});
		registerInputProcessor("info caution_factor",[this](InputListener &listener) { this->info_caution_factor(listener);});
		registerInputProcessor("info pondering",	[this](InputListener &listener) { this->info_pondering(listener);});
		registerInputProcessor("info usedatabase",	[this](InputListener &listener) { this->info_usedatabase(listener);});
		registerInputProcessor("info thread_num",	[this](InputListener &listener) { this->info_thread_num(listener);});
		registerInputProcessor("info nbest_sym",	[this](InputListener &listener) { this->info_nbest_sym(listener);});
		registerInputProcessor("info checkmate",	[this](InputListener &listener) { this->info_checkmate(listener);});
		registerInputProcessor("info rule",			[this](InputListener &listener) { this->info_rule(listener);});
		registerInputProcessor("info show_detail",	[this](InputListener &listener) { this->info_show_detail(listener);});

		registerInputProcessor("start",		   [this](InputListener &listener) { this->start(listener);});
		registerInputProcessor("yxboard",	   [this](InputListener &listener) { this->yxboard(listener);});
		registerInputProcessor("yxstop",	   [this](InputListener &listener) { this->yxstop(listener);});
		registerInputProcessor("yxshowforbid", [this](InputListener &listener) { this->yxshowforbid(listener);});
		registerInputProcessor("yxbalance",	   [this](InputListener &listener) { this->yxbalance(listener);});
		registerInputProcessor("yxnbest",	   [this](InputListener &listener) { this->yxnbest(listener);});
		// hashtable
		registerInputProcessor("yxhashclear",	  [this](InputListener &listener) { this->yxhashclear(listener);});
		registerInputProcessor("yxhashdump",	  [this](InputListener &listener) { this->yxhashdump(listener);});
		registerInputProcessor("yxhashload",	  [this](InputListener &listener) { this->yxhashload(listener);});
		registerInputProcessor("yxshowhashusage", [this](InputListener &listener) { this->yxshowhashusage(listener);});
		// openings
		registerInputProcessor("yxswap2",	[this](InputListener &listener) { this->yxswap2(listener);});
		registerInputProcessor("yxsoosorv",	[this](InputListener &listener) { this->yxsoosorv(listener);});
		// search
		registerInputProcessor("yxdraw",			[this](InputListener &listener) { this->yxdraw(listener);});
		registerInputProcessor("yxresign",			[this](InputListener &listener) { this->yxresign(listener);});
		registerInputProcessor("yxshowinfo",		[this](InputListener &listener) { this->yxshowinfo(listener);});
		registerInputProcessor("yxprintfeature",	[this](InputListener &listener) { this->yxprintfeature(listener);});
		registerInputProcessor("yxblockpathreset",	[this](InputListener &listener) { this->yxblockpathreset(listener);});
		registerInputProcessor("yxblockpathundo",	[this](InputListener &listener) { this->yxblockpathundo(listener);});
		registerInputProcessor("yxblockpath",		[this](InputListener &listener) { this->yxblockpath(listener);});
		registerInputProcessor("yxblockreset",		[this](InputListener &listener) { this->yxblockreset(listener);});
		registerInputProcessor("yxblockundo",		[this](InputListener &listener) { this->yxblockundo(listener);});
		registerInputProcessor("yxsearchdefend",	[this](InputListener &listener) { this->yxsearchdefend(listener);});
		// database
		registerInputProcessor("yxsetdatabase",			[this](InputListener &listener) { this->yxsetdatabase(listener);});
		registerInputProcessor("yxquerydatabaseall",	[this](InputListener &listener) { this->yxquerydatabaseall(listener);});
		registerInputProcessor("yxquerydatabaseone",	[this](InputListener &listener) { this->yxquerydatabaseone(listener);});
		registerInputProcessor("yxeditlabeldatabase",	[this](InputListener &listener) { this->yxeditlabeldatabase(listener);});
		registerInputProcessor("yxedittvddatabase",		[this](InputListener &listener) { this->yxedittvddatabase(listener);});
		registerInputProcessor("yxdeletedatabaseone",	[this](InputListener &listener) { this->yxdeletedatabaseone(listener);});
		registerInputProcessor("yxdeletedatabaseall",	[this](InputListener &listener) { this->yxdeletedatabaseall(listener);});
		registerInputProcessor("yxsetbestmovedatabase",	[this](InputListener &listener) { this->yxsetbestmovedatabase(listener);});
		registerInputProcessor("yxclearbestmovedatabase",[this](InputListener &listener) { this->yxclearbestmovedatabase(listener);});
		registerInputProcessor("yxdbtopos",				[this](InputListener &listener) { this->yxdbtopos(listener);});
		registerInputProcessor("yxdbtotxt",				[this](InputListener &listener) { this->yxdbtotxt(listener);});
		registerInputProcessor("yxtxttodb",				[this](InputListener &listener) { this->yxtxttodb(listener);});
		registerInputProcessor("yxdbcheck",				[this](InputListener &listener) { this->yxdbcheck(listener);});
		registerInputProcessor("yxdbfix",				[this](InputListener &listener) { this->yxdbfix(listener);});
// @formatter:on
	}
	void YixinBoardProtocol::reset()
	{
		GomocupProtocol::reset();
	}
	ProtocolType YixinBoardProtocol::getType() const noexcept
	{
		return ProtocolType::YIXINBOARD;
	}
	/*
	 * private
	 */
	void YixinBoardProtocol::best_move(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		if (opening_swap2)
		{
			switch (list_of_moves.size())
			{
				case 0:
				{
					for (size_t i = 0; i < msg.getListOfMoves().size(); i++)
						sender.send("MESSAGE SWAP2 MOVE" + std::to_string(1 + i) + " " + move_yx_encoding(msg.getListOfMoves().at(i)));
					break;
				}
				case 3:
				{
					if (msg.holdsString() and msg.getString() == "swap") // used to swap colors
						sender.send("MESSAGE SWAP2 SWAP1 YES");
					if (msg.holdsMove()) // used to return the best move
						sender.send("MESSAGE SWAP2 SWAP1 NO");
					if (msg.holdsListOfMoves()) // used to return multiple moves (for example an opening)
						for (size_t i = 0; i < msg.getListOfMoves().size(); i++)
							sender.send("MESSAGE SWAP2 MOVE" + std::to_string(4 + i) + " " + move_yx_encoding(msg.getListOfMoves().at(i)));
					break;
				}
				case 5:
				{
					if (msg.holdsString() and msg.getString() == "swap") // used to swap colors
						sender.send("MESSAGE SWAP2 SWAP2 YES");
					if (msg.holdsMove()) // used to return the best move
						sender.send("MESSAGE SWAP2 SWAP2 NO");
					break;
				}
			}
			opening_swap2 = false;
		}
		if (opening_soosorv)
		{

		}
		if (msg.holdsMove()) // used to return the best move
		{
			assert(msg.getMove().sign == get_sign_to_move());
			sender.send(moveToString(msg.getMove()));
			list_of_moves.push_back(msg.getMove());
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

	void YixinBoardProtocol::info_max_depth(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_depth", extract_command_data(listener, "info max_depth") }));
	}
	void YixinBoardProtocol::info_max_node(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_nodes", extract_command_data(listener, "info max_node") }));
	}
	void YixinBoardProtocol::info_time_increment(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "time_increment", extract_command_data(listener, "info time_increment") }));
	}
	void YixinBoardProtocol::info_caution_factor(InputListener &listener)
	{
		const int cf = 4 - std::stoi(extract_command_data(listener, "info caution_factor")); // value must be inverted as in AG higher values correspond to more aggressive style
		input_queue.push(Message(MessageType::SET_OPTION, Option { "style", std::to_string(cf) }));
	}
	void YixinBoardProtocol::info_pondering(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "auto_pondering", extract_command_data(listener, "info pondering") }));
	}
	void YixinBoardProtocol::info_usedatabase(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "use_database", extract_command_data(listener, "info usedatabase") }));
	}
	void YixinBoardProtocol::info_thread_num(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "thread_num", extract_command_data(listener, "info thread_num") }));
	}
	void YixinBoardProtocol::info_nbest_sym(InputListener &listener)
	{
		nbest = std::stoi(extract_command_data(listener, "info nbest_sym"));
	}
	void YixinBoardProtocol::info_checkmate(InputListener &listener)
	{
		search_type = std::stoi(extract_command_data(listener, "info checkmate"));
	}
	void YixinBoardProtocol::info_rule(InputListener &listener)
	{
		const std::string data = extract_command_data(listener, "info rule");
		is_renju_rule = (std::stoi(data) == 2);
		switch (std::stoi(data))
		{
			case 0:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::FREESTYLE) }));
				break;
			case 1:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::STANDARD) }));
				break;
			case 2:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) }));
				break;
			default:
				output_queue.push(Message(MessageType::ERROR, "Invalid rule " + data));
				break;
		}
	}
	void YixinBoardProtocol::info_show_detail(InputListener &listener)
	{
		show_realtime_info = (std::stoi(extract_command_data(listener, "info show_detail")) == 1);
	}

	void YixinBoardProtocol::start(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');

		int size = -1;
		switch (tmp.size())
		{
			case 2:
			{
				size = std::stoi(tmp.at(1));
				break;
			}
			case 3:
			{
				if (std::stoi(tmp.at(1)) == std::stoi(tmp.at(2))) // actually a square board
					size = std::stoi(tmp.at(1));
				else
					output_queue.push(Message(MessageType::ERROR, "Rectangular boards are not supported"));
				break;
			}
			default:
				throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");
		}

		if (size == 15 or size == 20)
		{
			setup_board_size(size, size);
			input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", std::to_string(size) }));
			input_queue.push(Message(MessageType::SET_OPTION, Option { "columns", std::to_string(size) }));
			input_queue.push(Message(MessageType::START_PROGRAM));
			output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Only 15x15 or 20x20 boards are supported"));
	}
	void YixinBoardProtocol::yxboard(InputListener &listener)
	{
		listener.consumeLine("yxboard");
		list_of_moves = parse_list_of_moves(listener, "done");
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
	}
	void YixinBoardProtocol::yxstop(InputListener &listener)
	{
		listener.consumeLine("yxstop");
		input_queue.push(Message(MessageType::STOP_SEARCH));
	}
	void YixinBoardProtocol::yxshowforbid(InputListener &listener)
	{
		listener.consumeLine("yxshowforbid");
		if (is_renju_rule)
		{
			const matrix<Sign> board = Board::fromListOfMoves(rows, columns, list_of_moves);
			std::string response = "FORBID ";
			for (int r = 0; r < board.rows(); r++)
				for (int c = 0; c < board.cols(); c++)
					if (isForbidden(board, Move(r, c, Sign::CROSS)))
						response += ag::zfill(c, 2) + ag::zfill(r, 2);
			output_queue.push(Message(MessageType::PLAIN_STRING, response));
		}
	}
	void YixinBoardProtocol::yxbalance(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 3u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		int n;
		if (tmp.at(1) == "one")
			n = 1;
		else
		{
			if (tmp.at(1) == "two")
				n = 2;
			else
				output_queue.push(Message(MessageType::ERROR, "Unsupported number of moves '" + tmp.at(1) + "'"));
		}
		// TODO what to do with tmp[2] ???
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::START_SEARCH, "balance " + std::to_string(n)));
	}
	void YixinBoardProtocol::yxnbest(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 3u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		input_queue.push(Message(MessageType::STOP_SEARCH));
//		input_queue.push(Message(MessageType::START_SEARCH, "balance " + std::to_string(n)));
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxnbest'"));
	}
	/*
	 * hashtable
	 */
	void YixinBoardProtocol::yxhashclear(InputListener &listener)
	{
		listener.consumeLine("yxhashclear");
		std::vector<Move> tmp;
		for (int r = 0; r < rows; r++)
			for (int c = 0; c < columns; c++)
				tmp.push_back(Move(Sign::CIRCLE, r, c)); // creating impossible board with all circles (could also be all crosses)

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, tmp)); // Node cache deletes states that are provably not possible given current board state. Given an impossible board state, all nodes will be cleared.
		input_queue.push(Message(MessageType::START_SEARCH, "clearhash")); // launching search with special controller that will pass this board to the engine causing tree reset but without actually starting the search.
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves)); // reverting to the original board state
	}
	void YixinBoardProtocol::yxhashdump(InputListener &listener)
	{
		listener.consumeLine("yxhashdump");
		const std::string path = listener.getLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxhashdump'"));
	}
	void YixinBoardProtocol::yxhashload(InputListener &listener)
	{
		listener.consumeLine("yxhashload");
		const std::string path = listener.getLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxhashload'"));
	}
	void YixinBoardProtocol::yxshowhashusage(InputListener &listener)
	{
		listener.consumeLine("yxshowhashusage");
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxshowhashusage'"));
	}
	/*
	 * opening rules
	 */
	void YixinBoardProtocol::yxswap2(InputListener &listener)
	{
		const std::string line = listener.getLine();
		opening_swap2 = true;
		if (line == "yxswap2step1")
		{
		}
		if (line == "yxswap2step2")
		{
		}
		if (line == "yxswap2step3")
		{
		}

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves)); // current board position will be set by yxboard command before swap2 commands
		input_queue.push(Message(MessageType::START_SEARCH, "swap2"));
	}
	void YixinBoardProtocol::yxsoosorv(InputListener &listener)
	{
		const std::string line = listener.getLine();
//		opening_soosorv = true;
//		if (line == "yxsoosorvstep1")
//		{
//		}
//		if (line == "yxsoosorvstep2")
//		{
//		}
//		if (line == "yxsoosorvstep3")
//		{
//		}
//		if (line == "yxsoosorvstep4")
//		{
//		}
//		if (line == "yxsoosorvstep5")
//		{
//		}
//		if (line == "yxsoosorvstep6")
//		{
//		}
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxsoosorv'"));
	}
	/*
	 * search
	 */
	void YixinBoardProtocol::yxdraw(InputListener &listener)
	{
		listener.consumeLine("yxdraw");
		output_queue.push(Message(MessageType::INFO_MESSAGE, "DRAW REJECT")); // TODO can also be "DRAW ACCEPT" but for now always reject as protocol cannot directly ask the engine about the decision
	}
	void YixinBoardProtocol::yxresign(InputListener &listener)
	{
		listener.consumeLine("yxresign");
		output_queue.push(Message(MessageType::PLAIN_STRING, "Thanks :)"));
	}
	void YixinBoardProtocol::yxshowinfo(InputListener &listener)
	{
		// this is a good command, but it should be used to print description of all INFO commands supported by the engine
		listener.consumeLine("yxshowinfo");
		output_queue.push(Message(MessageType::INFO_MESSAGE, "INFO MAX_THREAD " + std::to_string(ml::Device::numberOfCpuCores())));
		output_queue.push(Message(MessageType::INFO_MESSAGE, "INFO MAX_HASH_SIZED 20")); // in MCTS engine there is no hash size so we can return anything here
	}
	void YixinBoardProtocol::yxprintfeature(InputListener &listener)
	{
		listener.consumeLine("yxprintfeature");
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxprintfeature'"));
	}
	void YixinBoardProtocol::yxblockpathreset(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxblockpathreset'"));
	}
	void YixinBoardProtocol::yxblockpathundo(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxblockpathundo'"));
	}
	void YixinBoardProtocol::yxblockpath(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxblockpath'"));
	}
	void YixinBoardProtocol::yxblockreset(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxblockreset'"));
	}
	void YixinBoardProtocol::yxblockundo(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxblockundo'"));
	}
	void YixinBoardProtocol::yxsearchdefend(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxsearchdefend'"));
	}
	/*
	 * database
	 */
	void YixinBoardProtocol::yxsetdatabase(InputListener &listener)
	{
		listener.consumeLine("yxsetdatabase");
		std::string path = listener.getLine();
		// sets path to the database
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxsetdatabase'"));
	}
	void YixinBoardProtocol::yxquerydatabaseall(InputListener &listener)
	{
		listener.consumeLine("yxquerydatabaseall");
		consume_list_of_moves(listener, "done");
		// seek for board state in the database
//		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxquerydatabaseall'")); // TODO for now just silently ignore this command to not spam the log with errors
	}
	void YixinBoardProtocol::yxquerydatabaseone(InputListener &listener)
	{
		listener.consumeLine("yxquerydatabaseone");
		consume_list_of_moves(listener, "done");
		// seek for board state in the database
		// TODO how is this different from the above version ???
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxquerydatabaseone'"));
	}
	void YixinBoardProtocol::yxeditlabeldatabase(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxeditlabeldatabase'"));
	}
	void YixinBoardProtocol::yxedittvddatabase(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxedittvddatabase'"));
	}
	void YixinBoardProtocol::yxdeletedatabaseone(InputListener &listener)
	{
		listener.consumeLine("yxdeletedatabaseone");
		consume_list_of_moves(listener, "done");
		// deletes entry for given board state from the database
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdeletedatabaseone'"));
	}
	void YixinBoardProtocol::yxdeletedatabaseall(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		switch (tmp.size())
		{
			case 1: // no argument, delete all
			{
				break;
			}
			case 2: // one argument, delete specific entries
			{
				if (tmp.at(1) == "wl")
				{

				}
				else
				{
					if (tmp.at(1) == "nowl")
					{

					}
					else
						throw ProtocolRuntimeException("Incorrect command '" + line + "'");
				}
				break;
			}
			default:
				throw ProtocolRuntimeException("Incorrect command '" + line + "'");
		}
		// deletes multiple entries for given board state from the database
		list_of_moves = parse_list_of_moves(listener, "done"); // TODO why do we pass this ???
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdeletedatabaseall'"));
	}
	void YixinBoardProtocol::yxsetbestmovedatabase(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxsetbestmovedatabase'"));
	}
	void YixinBoardProtocol::yxclearbestmovedatabase(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxclearbestmovedatabase'"));
	}
	void YixinBoardProtocol::yxdbtopos(InputListener &listener)
	{
		listener.consumeLine("yxdbtopos");
		std::string path = listener.getLine();
		// saves database as pos (probably the format Xa1Ob5 etc.
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdbtopos'"));
	}
	void YixinBoardProtocol::yxdbtotxt(InputListener &listener)
	{
		listener.consumeLine("yxdbtotxt");
		std::string path = listener.getLine();
		// saves the database as text
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdbtotxt'"));
	}
	void YixinBoardProtocol::yxtxttodb(InputListener &listener)
	{
		listener.consumeLine("yxtxttodb");
		std::string path = listener.getLine();
		//loads the database from text representation
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxtxttodb'"));
	}
	void YixinBoardProtocol::yxdbcheck(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdbcheck'"));
	}
	void YixinBoardProtocol::yxdbfix(InputListener &listener)
	{
		listener.consumeLine();
		output_queue.push(Message(MessageType::ERROR, "Unsupported command 'yxdbfix'"));
	}

} /* namespace ag */

