/*
 * GomocupProtocol.cpp
 *
 *  Created on: Apr 5, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/version.hpp>

#include <libml/hardware/Device.hpp>

#include <cassert>

namespace
{
	std::string format_percents(double x)
	{
		int tmp = static_cast<int>(1000 * x);
		return std::to_string(tmp / 10) + '.' + std::to_string(tmp % 10);
	}
}

namespace ag
{

	GomocupProtocol::GomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT) :
			Protocol(queueIN, queueOUT)
	{
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
	void GomocupProtocol::processInput(InputListener &listener)
	{
		std::string line = listener.peekLine();

		std::lock_guard lock(protocol_mutex);

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
		std::lock_guard lock(protocol_mutex);

		while (output_queue.isEmpty() == false)
		{
			Message msg = output_queue.pop();
			switch (msg.getType())
			{
				case MessageType::BEST_MOVE:
				{
					if (msg.holdsMove()) // used to return the best move
					{
						assert(msg.getMove().sign == get_sign_to_move());
						sender.send(moveToString(msg.getMove()));
						list_of_moves.push_back(msg.getMove());
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
					"Move " + moveToString(move) + " is outside of " + std::to_string(rows) + "x" + std::to_string(columns) + " board");
		for (size_t i = 0; i < playedMoves.size(); i++)
			if (playedMoves[i].row == move.row and playedMoves[i].col == move.col)
				throw ProtocolRuntimeException("Spot " + moveToString(move) + " is already occupied");
	}
	std::string GomocupProtocol::parse_search_summary(const SearchSummary &summary) const
	{
		if (summary.node.getVisits() == 0)
			return "";
		std::string result;
		if (summary.principal_variation.size() > 0)
			result += "depth 1-" + std::to_string(summary.principal_variation.size());
		switch (summary.node.getProvenValue())
		{
			case ProvenValue::UNKNOWN:
				result += " ev " + format_percents(summary.node.getWinRate() + 0.5f * summary.node.getDrawRate());
				break;
			case ProvenValue::LOSS:
				result += " ev L";
				break;
			case ProvenValue::DRAW:
				result += " ev D";
				break;
			case ProvenValue::WIN:
				result += " ev W";
				break;
		}
		result += " winrate " + format_percents(summary.node.getWinRate());
		result += " drawrate " + format_percents(summary.node.getDrawRate());

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
			std::string line = listener.getLine();
			if (line == ending)
				break;
			std::vector<std::string> tmp = split(line, ',');
			if (tmp.size() != 3u)
				throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

			Move m(std::stoi(tmp.at(1)), std::stoi(tmp.at(0)));
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
				throw ProtocolRuntimeException("Invalid position - too many stones either color");
		}

		std::vector<Move> result;
		if (own_moves.size() != opp_moves.size())
		{
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
	void GomocupProtocol::INFO(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 3u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		if (tmp.at(1) == "timeout_turn")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_turn", tmp.at(2) }));
			return;
		}
		if (tmp.at(1) == "timeout_match")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_for_match", tmp.at(2) }));
			return;
		}
		if (tmp.at(1) == "time_left")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "time_left", tmp.at(2) }));
			return;
		}
		if (tmp.at(1) == "max_memory")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "max_memory", tmp.at(2) }));
			return;
		}
		if (tmp.at(1) == "game_type")
		{
			return;
		}
		if (tmp.at(1) == "rule")
		{
			switch (std::stoi(tmp.at(2)))
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
//					input_queue.push(Message(MessageType::SET_OPTION, Option { "rules", toString(GameRules::RENJU) })); TODO uncomment this once this rule is supported
					output_queue.push(Message(MessageType::ERROR, "Renju rule is not supported"));
					return;
				default:
					output_queue.push(Message(MessageType::ERROR, "Invalid rule " + tmp.at(2)));
					return;
			}
		}
		if (tmp.at(1) == "evaluate")
		{
			const Move m = moveFromString(tmp.at(2), Sign::NONE);
			input_queue.push(Message(MessageType::INFO_MESSAGE, std::vector<Move>( { m })));
			return;
		}
		if (tmp.at(1) == "folder")
		{
			input_queue.push(Message(MessageType::SET_OPTION, Option { "folder", tmp.at(2) }));
			return;
		}
		Logger::write("Unknown option '" + tmp.at(1) + "'");
	}
	void GomocupProtocol::START(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		input_queue.push(Message(MessageType::START_PROGRAM));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", tmp.at(1) }));
		input_queue.push(Message(MessageType::SET_OPTION, Option { "columns", tmp.at(1) }));

		if (std::stoi(tmp.at(1)) == 15 or std::stoi(tmp.at(1)) == 20)
		{
			rows = std::stoi(tmp.at(1));
			columns = std::stoi(tmp.at(1));
			output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Only 15x15 or 20x20 boards are supported"));
	}
	void GomocupProtocol::RECTSTART(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		tmp = split(tmp.at(1), ',');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		if (std::stoi(tmp.at(0)) == std::stoi(tmp.at(1))) // actually a square board
		{
			rows = std::stoi(tmp.at(1));
			columns = std::stoi(tmp.at(0));
			input_queue.push(Message(MessageType::SET_OPTION, Option { "rows", tmp.at(1) }));
			input_queue.push(Message(MessageType::SET_OPTION, Option { "columns", tmp.at(0) }));
			input_queue.push(Message(MessageType::START_PROGRAM));
		}
		else
			output_queue.push(Message(MessageType::ERROR, "Rectangular boards are not supported"));
	}
	void GomocupProtocol::RESTART(InputListener &listener)
	{
		listener.consumeLine("RESTART");
		input_queue.push(Message(MessageType::START_PROGRAM));
		output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
	}
	void GomocupProtocol::BEGIN(InputListener &listener)
	{
		listener.consumeLine("BEGIN");

		list_of_moves.clear();
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::BOARD(InputListener &listener)
	{
		listener.consumeLine("BOARD");
		list_of_moves = parse_list_of_moves(listener, "DONE");
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::TURN(InputListener &listener)
	{
		std::string line = listener.getLine();
		std::vector<std::string> tmp = split(line, ' ');
		if (tmp.size() != 2u)
			throw ProtocolRuntimeException("Incorrect command '" + line + "' was passed");

		const Move new_move = moveFromString(tmp.at(1), get_sign_to_move());
		add_new_move(new_move);
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
		input_queue.push(Message(MessageType::START_SEARCH, "bestmove"));
	}
	void GomocupProtocol::TAKEBACK(InputListener &listener)
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
			const Move move_to_takeback = moveFromString(tmp.at(1), last_move.sign);
			if (last_move != move_to_takeback)
				output_queue.push(Message(MessageType::ERROR, "Can undo only the last move which is " + moveToString(last_move)));
			else
			{
				list_of_moves.pop_back();
				input_queue.push(Message(MessageType::STOP_SEARCH));
				input_queue.push(Message(MessageType::SET_POSITION, list_of_moves));
				output_queue.push(Message(MessageType::PLAIN_STRING, "OK"));
			}
		}
	}
	void GomocupProtocol::END(InputListener &listener)
	{
		listener.consumeLine("END");
		input_queue.push(Message(MessageType::STOP_SEARCH));
		input_queue.push(Message(MessageType::EXIT_PROGRAM));
	}
	void GomocupProtocol::ABOUT(InputListener &listener)
	{
		listener.consumeLine("ABOUT");
		std::string result;
		result += "name=\"" + ProgramInfo::name() + "\", ";
		result += "version=\"" + ProgramInfo::version() + "\", ";
		result += "author=\"" + ProgramInfo::author() + "\", ";
		result += "country=\"" + ProgramInfo::country() + "\", ";
		result += "www=\"" + ProgramInfo::website() + "\", ";
		result += "email=\"" + ProgramInfo::email() + "\"";
		output_queue.push(Message(MessageType::ABOUT_ENGINE, result));
	}
	void GomocupProtocol::UNKNOWN(InputListener &listener)
	{
		std::string line = listener.getLine();
		output_queue.push(Message(MessageType::UNKNOWN_COMMAND, line));
	}
} /* namespace ag */

