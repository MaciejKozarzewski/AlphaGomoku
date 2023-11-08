/*
 * YixinBoardProtocol.cpp
 *
 *  Created on: Jun 13, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/YixinBoardProtocol.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
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
		return std::to_string(m.row) + " " + std::to_string(m.col);
	}
	std::string move_yx_text(ag::Move m, int boardSize)
	{
		return static_cast<char>(static_cast<int>('A') + m.col) + std::to_string(boardSize - m.row);
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
		transpose_coords = false;
		// @formatter:off
		registerOutputProcessor(MessageType::BEST_MOVE, 	[this](OutputSender &sender) { this->best_move(sender);});
		registerOutputProcessor(MessageType::INFO_MESSAGE,	[this](OutputSender &sender) { this->info_message(sender);});

		registerInputProcessor("info max_depth", 	[this](InputListener &listener) { this->info_max_depth(listener);});
		registerInputProcessor("info max_node",		[this](InputListener &listener) { this->info_max_node(listener);});
		registerInputProcessor("info time_increment",[this](InputListener &listener) { this->info_time_increment(listener);});
		registerInputProcessor("info caution_factor",[this](InputListener &listener) { this->info_caution_factor(listener);});
		registerInputProcessor("info pondering",	[this](InputListener &listener) { this->info_pondering(listener);});
		registerInputProcessor("info usedatabase",	[this](InputListener &listener) { this->info_usedatabase(listener);});
		registerInputProcessor("info thread_num",	[this](InputListener &listener) { this->info_thread_num(listener);});
		registerInputProcessor("info nbest_sym",	[this](InputListener &listener) { this->info_nbest_sym(listener);});
		registerInputProcessor("info checkmate",	[this](InputListener &listener) { this->info_checkmate(listener);});
		registerInputProcessor("info hash_size",	[this](InputListener &listener) { this->info_hash_size(listener);});
		registerInputProcessor("info thread_split_depth",[this](InputListener &listener) { this->info_thread_split_depth(listener);});
		registerInputProcessor("info rule",			[this](InputListener &listener) { this->info_rule(listener);});
		registerInputProcessor("info show_detail",	[this](InputListener &listener) { this->info_show_detail(listener);});

		registerInputProcessor("start",		   [this](InputListener &listener) { this->start(listener);});
		registerInputProcessor("begin",		   [this](InputListener &listener) { this->begin(listener);});
		registerInputProcessor("board",		   [this](InputListener &listener) { this->board(listener);});
		registerInputProcessor("turn",		   [this](InputListener &listener) { this->turn(listener);});
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
	YixinBoardProtocol::~YixinBoardProtocol()
	{
		stop_realtime_handler();
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
		stop_realtime_handler();

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
			return;
		}
		if (opening_soosorv)
		{
			return;
		}
		if (msg.holdsMove()) // used to return the best move
		{
			assert(msg.getMove().sign == get_sign_to_move());
			sender.send(move_to_string(msg.getMove()));
			list_of_moves.push_back(msg.getMove());
			return;
		}
		if (msg.holdsListOfMoves()) // used to return multiple moves (for example an opening)
		{ // TODO not sure it this is use anywhere within YixinBoard protocol
			std::string str;
			for (size_t i = 0; i < msg.getListOfMoves().size(); i++)
			{
				if (i != 0)
					str += ' ';
				str += move_to_string(msg.getListOfMoves().at(i));
			}
			sender.send(str);
			return;
		}
	}
	void YixinBoardProtocol::info_message(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		if (msg.holdsString())
			sender.send("MESSAGE " + msg.getString());
		if (msg.holdsSearchSummary())
		{
			const SearchSummary &summary = msg.getSearchSummary();
			if (summary.principal_variation.size() == 0)
				process_realtime_info(summary, sender);
			else
			{
				if (summary.node.getVisits() > 0)
				{
					std::string info = "Depth: 1-" + std::to_string(summary.principal_variation.size());
					info += " | Time: " + std::to_string((int) (1000 * summary.time_used)) + "MS";
					info += " | Node: " + std::to_string(summary.number_of_nodes);
					sender.send("MESSAGE " + info);

					if (summary.time_used > 0.0)
						info = "Speed: " + std::to_string((int) (summary.number_of_nodes / summary.time_used));
					else
						info = "Speed: 0";
					switch (summary.node.getScore().getProvenValue())
					{
						case ProvenValue::UNKNOWN:
							info += " | Evaluation: " + format_percents(summary.node.getExpectation());
							break;
						case ProvenValue::LOSS:
						case ProvenValue::DRAW:
						case ProvenValue::WIN:
							info += " | Evaluation: " + trim(summary.node.getScore().toFormattedString());
							break;
					}
					sender.send("MESSAGE " + info);

					info = "Bestline:";
					for (size_t i = 0; i < summary.principal_variation.size(); i++)
						info += " [" + move_yx_text(summary.principal_variation[i], rows) + "]";
					sender.send("MESSAGE " + info);
				}
			}
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
		listener.consumeLine();
//		const int cf = 4 - std::stoi(extract_command_data(listener, "info caution_factor")); // value must be inverted as in AG higher values correspond to more aggressive style
//		input_queue.push(Message(MessageType::SET_OPTION, Option { "style", std::to_string(cf) }));
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
	void YixinBoardProtocol::info_hash_size(InputListener &listener)
	{
		const int hash_size = std::stoi(extract_command_data(listener, "info hash_size"));
		if (hash_size < 256)
			output_queue.push(Message(MessageType::INFO_MESSAGE, "Minimum value of 'hash_size' is 8 (256MB)"));

		const uint64_t max_memory = std::max(256, hash_size); // [MB]
		output_queue.push(Message(MessageType::INFO_MESSAGE, "Using " + std::to_string(max_memory) + "MB of memory"));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_memory", std::to_string(max_memory * 1024 * 1024) }));
	}
	void YixinBoardProtocol::info_thread_split_depth(InputListener &listener)
	{
		listener.consumeLine(); // this option does not make sense in MCTS
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
	void YixinBoardProtocol::begin(InputListener &listener)
	{
		GomocupProtocol::begin(listener);
		start_realtime_handler();
	}
	void YixinBoardProtocol::board(InputListener &listener)
	{
		GomocupProtocol::board(listener);
		start_realtime_handler();
	}
	void YixinBoardProtocol::turn(InputListener &listener)
	{
		GomocupProtocol::turn(listener);
		start_realtime_handler();
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
						response += ag::zfill(r, 2) + ag::zfill(c, 2);
			response += '.';
			output_queue.push(Message(MessageType::PLAIN_STRING, response));
		}
	}
	void YixinBoardProtocol::yxbalance(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 3u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		int n = 0;
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
		start_realtime_handler();
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
			list_of_moves.clear(); // YixinBoard may not send 'board' command before starting new swap2 game
//			yxswap2step1
//			93452477751 : Answered : MESSAGE SWAP2 MOVE1 3 7
//			93452477765 : Answered : MESSAGE SWAP2 MOVE2 6 7
//			93452477768 : Answered : MESSAGE SWAP2 MOVE3 8 7
		}
		if (line == "yxswap2step2")
		{
//			93846343762 : Received : yxboard
//			93846343766 : Received : 7,6,2
//			93846343771 : Received : 6,7,1
//			93846343775 : Received : 5,6,2
//			93846343779 : Received : done
//			93846343785 : Received : yxswap2step2
//			93853002978 : Answered : MESSAGE SWAP2 MOVE4 6 9
//			93853002995 : Answered : MESSAGE SWAP2 MOVE5 8 8

//			94311848364 : Answered : MESSAGE SWAP2 SWAP1 YES
		}
		if (line == "yxswap2step3")
		{
//			94074673973 : Received : yxboard
//			94074673977 : Received : 5,5,2
//			94074673981 : Received : 8,7,1
//			94074673985 : Received : 9,8,2
//			94074673989 : Received : 7,6,1
//			94074673993 : Received : 6,5,2
//			94074673997 : Received : done
//			94074674002 : Received : yxswap2step3

//			94079443542 : Answered : MESSAGE SWAP2 SWAP2 YES
//			94191989713 : Answered : MESSAGE SWAP2 SWAP2 NO
		}

		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves)); // current board position will be set by yxboard command before swap2 commands
		input_queue.push(Message(MessageType::START_SEARCH, "swap2"));
	}
	void YixinBoardProtocol::yxsoosorv(InputListener &listener)
	{
		const std::string line = listener.getLine();
		opening_soosorv = true;
		if (line == "yxsoosorvstep1")
		{
//			96750807944 : Received : yxsoosorvstep1
//			96750808033 : Answered : MESSAGE SOOSORV MOVE1 7 7
//			96750808049 : Answered : MESSAGE SOOSORV MOVE2 6 7
//			96750808052 : Answered : MESSAGE SOOSORV MOVE3 5 7
		}
		if (line == "yxsoosorvstep2")
		{
		}
		if (line == "yxsoosorvstep3")
		{
//			96754877762 : Received : yxsoosorvstep3
//			96754877779 : Received : 7,7
//			96754877781 : Received : 6,7
//			96754877783 : Received : 5,7
//			96754877784 : Received : done
		}
		if (line == "yxsoosorvstep4")
		{
		}
		if (line == "yxsoosorvstep5")
		{
		}
		if (line == "yxsoosorvstep6")
		{
		}
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
		const int max_thread_num = ml::Device::numberOfCpuCores();
		const int max_hash_size = 20;

		output_queue.push(Message(MessageType::INFO_MESSAGE, "INFO MAX_THREAD_NUM " + std::to_string(max_thread_num)));
		output_queue.push(Message(MessageType::INFO_MESSAGE, "INFO MAX_HASH_SIZE " + std::to_string(max_hash_size)));
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

	bool YixinBoardProtocol::is_realtime_handler_running() const noexcept
	{
		std::lock_guard<std::mutex> lock(rm_mutex);
		return is_running;
	}
	void YixinBoardProtocol::start_realtime_handler()
	{
		if (not show_realtime_info)
			return;
		stop_realtime_handler();

		waiting_for_first_info = true;
		losing_moves.clear();
		previous_best_move = Move();
		{ /* artificial scope for lock */
			std::lock_guard<std::mutex> lock(rm_mutex);
			is_running = true;
		}

		rm_future = std::async(std::launch::async, [&]()
		{
			try
			{
				while (is_realtime_handler_running())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000 * wait_time)));
					if(is_realtime_handler_running())
					{
						input_queue.push(Message(MessageType::INFO_MESSAGE, std::vector<Move>()));
					}
				}
			}
			catch(std::exception &e)
			{
				ag::Logger::write(std::string("Realtime message handler encountered exception: ") + e.what());
			}
		});
	}
	void YixinBoardProtocol::stop_realtime_handler()
	{
		waiting_for_first_info = false;
		{ /* artificial scope for lock */
			std::lock_guard<std::mutex> lock(rm_mutex);
			is_running = false;
		}

		if (rm_future.valid())
			rm_future.wait();
	}
	void YixinBoardProtocol::process_realtime_info(const SearchSummary &summary, OutputSender &sender)
	{
		if (summary.node.numberOfEdges() == 0 or not is_realtime_handler_running())
			return;

		if (waiting_for_first_info)
		{
			sender.send("MESSAGE REALTIME REFRESH");
			for (auto edge = summary.node.begin(); edge < summary.node.end(); edge++)
			{
				const Move m = edge->getMove();
				sender.send("MESSAGE REALTIME POS " + move_to_string(m));
				sender.send("MESSAGE REALTIME DONE " + move_to_string(m));
			}
			waiting_for_first_info = false;
		}

		for (auto edge = summary.node.begin(); edge < summary.node.end(); edge++)
			if (edge->getProvenValue() == ProvenValue::LOSS)
			{
				const Move m = edge->getMove();
				if (std::find(losing_moves.begin(), losing_moves.end(), m) == losing_moves.end())
				{ // found new losing move
					sender.send("MESSAGE REALTIME LOSE " + move_to_string(m));
					losing_moves.push_back(m);
				}
			}

		EdgeSelectorConfig config;
		config.exploration_constant = 0.2f;
		LCBSelector selector(config);
		const Move best_move = selector.select(&summary.node)->getMove();
		if (best_move != previous_best_move)
		{
			sender.send("MESSAGE REALTIME BEST " + move_to_string(best_move));
			previous_best_move = best_move;
		}
	}

} /* namespace ag */

