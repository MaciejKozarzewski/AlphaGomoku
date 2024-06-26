/*
 * GomocupProtocol.cpp
 *
 *  Created on: Apr 5, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/version.hpp>

#include <cassert>

namespace
{
	using namespace ag;

	template<typename T>
	std::string get_evaluation_string(const T &x)
	{
		std::string result;
		switch (x.getScore().getProvenValue())
		{
			case ProvenValue::UNKNOWN:
				result += "ev " + format_percents(x.getExpectation());
				break;
			case ProvenValue::LOSS:
			case ProvenValue::DRAW:
			case ProvenValue::WIN:
				result += "ev " + trim(x.getScore().toFormattedString());
				break;
		}
		result += " winrate " + format_percents(x.getWinRate());
		result += " drawrate " + format_percents(x.getDrawRate());
		return result;
	}
}

namespace ag
{

	GomocupProtocol::GomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			Protocol(queueIN, queueOUT)
	{
// @formatter:off
		registerOutputProcessor(MessageType::BEST_MOVE,       [this](OutputSender &sender) { this->best_move(sender);});
		registerOutputProcessor(MessageType::PLAIN_STRING,    [this](OutputSender &sender) { this->plain_string(sender);});
		registerOutputProcessor(MessageType::UNKNOWN_COMMAND, [this](OutputSender &sender) { this->unknown_command(sender);});
		registerOutputProcessor(MessageType::ERROR,           [this](OutputSender &sender) { this->error(sender);});
		registerOutputProcessor(MessageType::INFO_MESSAGE,    [this](OutputSender &sender) { this->info_message(sender);});
		registerOutputProcessor(MessageType::ABOUT_ENGINE,    [this](OutputSender &sender) { this->about_engine(sender);});

		registerInputProcessor("info timeout_turn", [this](InputListener &listener) { this->info_timeout_turn(listener);});
		registerInputProcessor("info timeout_match",[this](InputListener &listener) { this->info_timeout_match(listener);});
		registerInputProcessor("info time_left", 	[this](InputListener &listener) { this->info_time_left(listener);});
		registerInputProcessor("info max_memory", 	[this](InputListener &listener) { this->info_max_memory(listener);});
		registerInputProcessor("info game_type",	[this](InputListener &listener) { this->info_game_type(listener);});
		registerInputProcessor("info rule",			[this](InputListener &listener) { this->info_rule(listener);});
		registerInputProcessor("info evaluate",		[this](InputListener &listener) { this->info_evaluate(listener);});
		registerInputProcessor("info folder",		[this](InputListener &listener) { this->info_folder(listener);});
		registerInputProcessor("start",				[this](InputListener &listener) { this->start(listener);});
		registerInputProcessor("rectstart",			[this](InputListener &listener) { this->rectstart(listener);});
		registerInputProcessor("restart",			[this](InputListener &listener) { this->restart(listener);});
		registerInputProcessor("begin",				[this](InputListener &listener) { this->begin(listener);});
		registerInputProcessor("board",				[this](InputListener &listener) { this->board(listener);});
		registerInputProcessor("turn",				[this](InputListener &listener) { this->turn(listener);});
		registerInputProcessor("takeback",			[this](InputListener &listener) { this->takeback(listener);});
		registerInputProcessor("end",				[this](InputListener &listener) { this->end(listener);});
		registerInputProcessor("about",				[this](InputListener &listener) { this->about(listener);});
		registerInputProcessor("unknown",			[this](InputListener &listener) { this->unknown(listener);});
// @formatter:on
	}
	void GomocupProtocol::reset()
	{
		std::lock_guard lock(protocol_mutex);
		list_of_moves.clear();
	}
	ProtocolType GomocupProtocol::getType() const noexcept
	{
		return ProtocolType::GOMOCUP;
	}
	/*
	 * private
	 */
	std::string GomocupProtocol::move_to_string(const ag::Move &m) const
	{
		if (transpose_coords)
			return std::to_string(m.col) + "," + std::to_string(m.row);
		else
			return std::to_string(m.row) + "," + std::to_string(m.col);
	}
	ag::Move GomocupProtocol::move_from_string(const std::string &str, ag::Sign sign) const
	{
		std::vector<std::string> tmp = split(str, ',');
		if (tmp.size() < 2u)
			throw std::runtime_error("Incorrect move '" + str + "' was passed");

		const int row = std::stoi(tmp[0]);
		const int col = std::stoi(tmp[1]);
		if (row < 0 or row >= 128 or col < 0 or col >= 128)
			throw std::logic_error("Invalid move '" + str + "'");
		if (transpose_coords)
			return ag::Move(col, row, sign);
		else
			return ag::Move(row, col, sign);
	}
	std::string GomocupProtocol::extract_command_data(InputListener &listener, const std::string &command) const
	{
		std::string line = listener.getLine();
		assert(startsWith(line, command));
		line.erase(line.begin(), std::find_if_not(line.begin() + command.size(), line.end(), [](unsigned char ch)
		{	return std::isspace(ch);}));
		return line;
	}
	Sign GomocupProtocol::get_sign_to_move() const noexcept
	{
		if (list_of_moves.empty())
			return Sign::CROSS;
		else
			return invertSign(list_of_moves.back().sign);
	}
	void GomocupProtocol::add_new_move(Move move)
	{
		check_move_validity(move, list_of_moves);
		list_of_moves.push_back(move);
	}
	void GomocupProtocol::check_move_validity(Move move, const std::vector<Move> &playedMoves) const
	{
		if (move.row < 0 or move.row >= rows or move.col < 0 or move.col >= columns)
			throw ProtocolRuntimeException(
					"Move " + move_to_string(move) + " is outside of " + std::to_string(rows) + "x" + std::to_string(columns) + " board");
		for (size_t i = 0; i < playedMoves.size(); i++)
			if (playedMoves[i].row == move.row and playedMoves[i].col == move.col)
				throw ProtocolRuntimeException("Spot " + move_to_string(move) + " is already occupied");
	}
	void GomocupProtocol::setup_board_size(int rows, int columns) noexcept
	{
		this->rows = rows;
		this->columns = columns;
	}
	std::string GomocupProtocol::parse_search_summary(const SearchSummary &summary)
	{
		search_info = summary.node;
		copyEdgeInfo(search_info, summary.node);

		if (summary.node.getVisits() == 0)
			return "";

		std::string result;
		if (summary.principal_variation.size() > 0)
			result += "depth 1-" + std::to_string(summary.principal_variation.size());

		result += " " + get_evaluation_string(summary.node);

		result += " n " + std::to_string(summary.number_of_nodes);
		if (summary.time_used > 0.0)
			result += " n/s " + std::to_string((int) (summary.number_of_nodes / summary.time_used));
		else
			result += " n/s 0";
		result += " tm " + std::to_string((int) (1000 * summary.time_used));
		if (summary.principal_variation.size() > 0)
		{
			result += " pv";
			for (size_t i = 0; i < summary.principal_variation.size(); i++)
				result += " " + summary.principal_variation[i].text();
		}
		return result;
	}
	std::vector<Move> GomocupProtocol::parse_list_of_moves(InputListener &listener, const std::string &ending) const
	{
		std::vector<Move> own_moves;
		std::vector<Move> opp_moves;
		while (true)
		{
			const std::string line = listener.getLine();
			if (line == ending)
				break;
			std::vector<std::string> tmp = split(line, ',');
			if (tmp.size() != 3u)
				throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

			const Move m = move_from_string(line, Sign::NONE);
			check_move_validity(m, own_moves);
			check_move_validity(m, opp_moves);

			switch (std::stoi(tmp.at(2)))
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
				throw ProtocolRuntimeException("Invalid position - too many stones of either color");
		}

		std::vector<Move> result;
		if (own_moves.size() != opp_moves.size())
		{ // move one own move to the result list, so in the next loop both lists have the same lengths
			result.push_back(opp_moves.front());
			opp_moves.erase(opp_moves.begin());
		}
		for (size_t i = 0; i < own_moves.size(); i++)
		{
			result.push_back(own_moves[i]);
			result.push_back(opp_moves[i]);
		}
		return result;
	}
	std::vector<Move> GomocupProtocol::parse_ordered_moves(InputListener &listener, const std::string &ending) const
	{
		std::vector<Move> result;
		Sign sign_to_move = Sign::CROSS;
		while (true)
		{
			const std::string line = listener.getLine();
			if (line == ending)
				break;
			std::vector<std::string> tmp = split(line, ',');
			if (tmp.size() != 2u)
				throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

			const Move m = move_from_string(line, sign_to_move);
			check_move_validity(m, result);
			result.push_back(m);
			sign_to_move = invertSign(sign_to_move);
		}
		return result;
	}
	/*
	 * Output processing
	 */
	void GomocupProtocol::best_move(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		if (msg.holdsMove()) // used to return the best move
		{
			assert(msg.getMove().sign == get_sign_to_move());
			sender.send(move_to_string(msg.getMove()));
			list_of_moves.push_back(msg.getMove());
		}
	}
	void GomocupProtocol::plain_string(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		sender.send(msg.getString());
	}
	void GomocupProtocol::unknown_command(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		sender.send("UNKNOWN '" + msg.getString() + "'");
	}
	void GomocupProtocol::error(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		sender.send("ERROR " + msg.getString());
	}
	void GomocupProtocol::info_message(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		if (msg.holdsString())
			sender.send("MESSAGE " + msg.getString());
		if (msg.holdsSearchSummary())
			sender.send("MESSAGE " + parse_search_summary(msg.getSearchSummary()));
	}
	void GomocupProtocol::about_engine(OutputSender &sender)
	{
		const Message msg = output_queue.pop();
		sender.send(msg.getString());
	}
	/*
	 * Input processing
	 */
	void GomocupProtocol::info_timeout_turn(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_turn", extract_command_data(listener, "info timeout_turn") }));
	}
	void GomocupProtocol::info_timeout_match(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_match", extract_command_data(listener, "info timeout_match") }));
	}
	void GomocupProtocol::info_time_left(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "time_left", extract_command_data(listener, "info time_left") }));
	}
	void GomocupProtocol::info_max_memory(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "max_memory", extract_command_data(listener, "info max_memory") }));
	}
	void GomocupProtocol::info_game_type(InputListener &listener)
	{
		listener.consumeLine();
	}
	void GomocupProtocol::info_rule(InputListener &listener)
	{
		const std::string data = extract_command_data(listener, "info rule");
		switch (std::stoi(data))
		{
			case 0:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::FREESTYLE) }));
				return;
			case 1:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::STANDARD) }));
				return;
			case 2:
				output_queue.push(Message(MessageType::ERROR, "Continuous game is not supported"));
				return;
			case 4:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) }));
				return;
			case 8:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO6) }));
				break;
			case 9:
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::CARO5) }));
				break;
			default:
				output_queue.push(Message(MessageType::ERROR, "Invalid rule " + data));
				return;
		}
	}
	void GomocupProtocol::info_evaluate(InputListener &listener)
	{
		const Move m = move_from_string(extract_command_data(listener, "info evaluate"), Sign::NONE);
		std::string result;
		if (search_info.numberOfEdges() > 0)
		{
			for (auto edge = search_info.begin(); edge < search_info.end(); edge++)
				if (edge->getMove().location() == m.location())
				{
					result = get_evaluation_string(*edge);
					break;
				}
		}
		output_queue.push(Message(MessageType::INFO_MESSAGE, result));
	}
	void GomocupProtocol::info_folder(InputListener &listener)
	{
		input_queue.push(Message(MessageType::SET_OPTION, Option { "folder", extract_command_data(listener, "info folder") }));
	}
	void GomocupProtocol::start(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const int size = std::stoi(tmp.at(1));

		input_queue.push(Message(MessageType::START_PROGRAM));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", std::to_string(size) }));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "columns", std::to_string(size) }));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "draw_after", std::to_string(square(size)) }));

		if (size == 15 or size == 20)
		{
			setup_board_size(size, size);
			output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Only 15x15 or 20x20 boards are supported"));
	}
	void GomocupProtocol::rectstart(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		tmp = split(tmp.at(1), ',');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const int tmp_rows = std::stoi(tmp.at(1));
		const int tmp_cols = std::stoi(tmp.at(0));

		if (tmp_rows == tmp_cols) // actually a square board
		{
			const int size = tmp_rows;
			if (size == 15 or size == 20)
			{
				setup_board_size(size, size);
				input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", std::to_string(size) }));
				input_queue.push(Message(MessageType::SET_OPTION, Option { "columns", std::to_string(size) }));
				input_queue.push(Message(MessageType::SET_OPTION, Option { "draw_after", std::to_string(square(size)) }));
				input_queue.push(Message(MessageType::START_PROGRAM));
				output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
			}
			else
				output_queue.push(Message(MessageType::ERROR, "Only 15x15 or 20x20 boards are supported"));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Rectangular boards are not supported"));
	}
	void GomocupProtocol::restart(InputListener &listener)
	{
		listener.consumeLine("restart");
		input_queue.push(Message(MessageType::START_PROGRAM));
		output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
	}
	void GomocupProtocol::begin(InputListener &listener)
	{
		listener.consumeLine("begin");
		list_of_moves.clear();
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::board(InputListener &listener)
	{
		listener.consumeLine("board");
		list_of_moves = parse_list_of_moves(listener, "done");
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::turn(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const Move new_move = move_from_string(tmp.at(1), get_sign_to_move());
		add_new_move(new_move);
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::takeback(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const int number_of_moves = list_of_moves.size();
		if (number_of_moves == 0)
			output_queue.push(Message(MessageType::ERROR, "The board is empty"));
		else
		{
			const Move last_move = list_of_moves.back();
			const Move move_to_takeback = move_from_string(tmp.at(1), last_move.sign);
			if (last_move != move_to_takeback)
				output_queue.push(Message(MessageType::ERROR, "Can undo only the last move which is " + move_to_string(last_move)));
			else
			{
				list_of_moves.pop_back();
				input_queue.push(Message(MessageType::STOP_SEARCH));
				input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
				output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
			}
		}
	}
	void GomocupProtocol::end(InputListener &listener)
	{
		listener.consumeLine("end");
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::EXIT_PROGRAM));
	}
	void GomocupProtocol::about(InputListener &listener)
	{
		listener.consumeLine("about");
		std::string result;
		result += "name=\"" + ProgramInfo::name() + "\", ";
		result += "version=\"" + ProgramInfo::version() + "\", ";
		result += "author=\"" + ProgramInfo::author() + "\", ";
		result += "country=\"" + ProgramInfo::country() + "\", ";
		result += "www=\"" + ProgramInfo::website() + "\", ";
		result += "email=\"" + ProgramInfo::email() + "\"";
		output_queue.push(Message(MessageType::ABOUT_ENGINE, result));
	}
	void GomocupProtocol::unknown(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}

} /* namespace ag */

