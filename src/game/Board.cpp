/*
 * Board.cpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <algorithm>
#include <iostream>

namespace
{
	using namespace ag;
	struct board_size
	{
			int rows;
			int cols;
	};
	board_size get_board_size_from_string(const std::string &str)
	{
		int height = std::count(str.begin(), str.end(), '\n');		// every line must end with new line character "\n"
		assert(str.size() % height == 0);
		int width = std::count(str.begin(), str.end(), ' ') / height; // every board spot must contain exactly one leading space
		return board_size { height, width };
	}
	ag::matrix<Sign> board_from_string(const std::string &str)
	{
		board_size size = get_board_size_from_string(str);

		assert(std::count(str.begin(), str.end(), ' ') == size.rows * size.cols);
		matrix<Sign> result(size.rows, size.cols);
		int counter = 0;
		for (size_t i = 0; i < str.size(); i++)
			switch (str.at(i))
			{
				case '_':
				case '!': // special case sometimes used in tests to indicate point of interest
					result.data()[counter] = Sign::NONE;
					counter++;
					break;
				case 'X':
					result.data()[counter] = Sign::CROSS;
					counter++;
					break;
				case 'O':
					result.data()[counter] = Sign::CIRCLE;
					counter++;
					break;
			}
		return result;
	}
//	std::vector<Move> extract_moves(const std::string &str)
//	{
//		board_size size = get_board_size_from_string(str);
//		std::vector<Move> result;
//		for (size_t i = 0; i < str.size(); i++)
//			if (str.at(i) == '!')
//				result.push_back(Move(i / size.cols, i % size.cols)); // FIXME this is incorrect
//		return result;
//	}

	std::string to_string(Sign s)
	{
		return " " + text(s);
	}
	std::string to_string(ProvenValue pv)
	{
		switch (pv)
		{
			default:
			case ProvenValue::UNKNOWN:
				return " _ ";
			case ProvenValue::LOSS:
				return ">L<";
			case ProvenValue::DRAW:
				return ">D<";
			case ProvenValue::WIN:
				return ">W<";
		}
	}
	std::string to_string(float f)
	{
		int t = static_cast<int>(1000 * f);
		if (t == 0)
			return "  _ ";
		else
		{
			if (t < 1000)
				return " " + std::to_string(t);
			else
			{
				if (t < 100)
					return "  " + std::to_string(t);
				else
					return "   " + std::to_string(t);
			}
		}
	}
	std::string to_string(Value v)
	{
		return to_string(v.win + 0.5f * v.draw);
	}

	std::string board_to_string(const matrix<Sign> &board)
	{
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
				result += to_string(board.at(i, j));
			result += '\n';
		}
		return result;
	}
	template<class T>
	std::string board_to_string(const ag::matrix<ag::Sign> &board, const T &other)
	{
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
					result += "  " + to_string(board.at(i, j)) + " ";
				else
					result += std::string(" ") + to_string(other.at(i, j));
			result += '\n';
		}
		return result;
	}
}

namespace ag
{
	std::string toString(Sign sign)
	{
		switch (sign)
		{
			default:
			case Sign::NONE:
				return "NONE  ";
			case Sign::CROSS:
				return "CROSS ";
			case Sign::CIRCLE:
				return "CIRCLE";
		}
	}
	std::string text(Sign sign)
	{
		switch (sign)
		{
			case Sign::NONE:
				return "_";
			case Sign::CROSS:
				return "X";
			case Sign::CIRCLE:
				return "O";
			default:
			case Sign::ILLEGAL:
				return "|";
		}
	}
	Sign signFromString(const std::string &str)
	{
		if (str == "CROSS")
			return Sign::CROSS;
		else
		{
			if (str == "CIRCLE")
				return Sign::CIRCLE;
			else
				return Sign::NONE;
		}
	}
	std::ostream& operator<<(std::ostream &stream, Sign sign)
	{
		stream << ag::toString(sign);
		return stream;
	}
	std::string operator+(const std::string &lhs, Sign rhs)
	{
		return lhs + ag::toString(rhs);
	}
	std::string operator+(Sign lhs, const std::string &rhs)
	{
		return ag::toString(lhs) + rhs;
	}

	std::string toString(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return "FREESTYLE";
			case GameRules::STANDARD:
				return "STANDARD";
			case GameRules::RENJU:
				return "RENJU";
			case GameRules::CARO:
				return "CARO";
			default:
				throw std::logic_error("unknown rule");
		}
	}
	GameRules rulesFromString(const std::string &str)
	{
		if (str == "FREESTYLE")
			return GameRules::FREESTYLE;
		if (str == "STANDARD")
			return GameRules::STANDARD;
		if (str == "RENJU")
			return GameRules::RENJU;
		if (str == "CARO")
			return GameRules::CARO;
		throw std::logic_error("unknown rule");
	}

	std::string toString(GameOutcome outcome)
	{
		switch (outcome)
		{
			default:
			case GameOutcome::UNKNOWN:
				return "UNKNOWN";
			case GameOutcome::DRAW:
				return "DRAW";
			case GameOutcome::CROSS_WIN:
				return "CROSS_WIN";
			case GameOutcome::CIRCLE_WIN:
				return "CIRCLE_WIN";
		}
	}
	GameOutcome outcomeFromString(const std::string &str)
	{
		if (str == "DRAW")
			return GameOutcome::DRAW;
		if (str == "CROSS_WIN")
			return GameOutcome::CROSS_WIN;
		if (str == "CIRCLE_WIN")
			return GameOutcome::CIRCLE_WIN;
		return GameOutcome::UNKNOWN;
	}

	Board::Board(int rows, int cols, GameRules rules) :
			board_data(rows, cols),
			sign_to_move(Sign::CROSS),
			game_rules(rules)
	{
	}
	Board::Board(const std::string &str, Sign signToMove, GameRules rules) :
			board_data(board_from_string(str)),
			sign_to_move(signToMove),
			game_rules(rules)
	{
	}

	Sign& Board::at(const std::string &text) noexcept
	{
		Move tmp('_' + text);
		return board_data.at(tmp.row, tmp.col);
	}
	Sign Board::at(const std::string &text) const noexcept
	{
		Move tmp('_' + text);
		return board_data.at(tmp.row, tmp.col);
	}

	void Board::putMove(Move move) noexcept
	{
		assert(move.sign == sign_to_move);
		assert(board_data.at(move.row, move.col) == Sign::NONE);

		board_data.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(move.sign);
	}
	void Board::undoMove(Move move) noexcept
	{
		assert(move.sign == invertSign(sign_to_move));
		assert(board_data.at(move.row, move.col) == move.sign);

		board_data.at(move.row, move.col) = Sign::NONE;
		sign_to_move = move.sign;
	}

	bool Board::isSquare() const noexcept
	{
		return rows() == cols();
	}
	bool Board::isEmpty() const noexcept
	{
		return std::all_of(board_data.begin(), board_data.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}
	bool Board::isFull() const noexcept
	{
		return std::none_of(board_data.begin(), board_data.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}
	bool Board::isForbidden(Move m) const noexcept
	{
		if (game_rules == GameRules::RENJU)
			return false; // TODO
		else
			return false; // if not Renju rule, no move is forbidden
	}
	GameOutcome Board::getOutcome() const noexcept
	{
		return GameOutcome::UNKNOWN; // TODO
	}
	GameOutcome Board::getOutcome(Move last_move) const noexcept
	{
		return GameOutcome::UNKNOWN; // TODO
	}

	std::string Board::toString() const
	{
		return board_to_string(board_data);
	}
	std::string Board::toString(const matrix<ProvenValue> &pv) const
	{
		return board_to_string(board_data, pv);
	}
	std::string Board::toString(const matrix<float> &policy) const
	{
		return board_to_string(board_data, policy);
	}
	std::string Board::toString(const matrix<Value> &actionValues) const
	{
		return board_to_string(board_data, actionValues);
	}

} /* namespace ag */

