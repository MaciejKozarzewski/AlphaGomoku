/*
 * Board.cpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>

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
		int height = std::count(str.begin(), str.end(), '\n'); // every line must end with new line character "\n"
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
					result += std::string(" ") + to_string(other.at(i, j));
				else
					result += "  " + to_string(board.at(i, j)) + " ";
			result += '\n';
		}
		return result;
	}
}

namespace ag
{
	Board::Board() :
			game_rules(GameRules::FREESTYLE),
			sign_to_move(Sign::NONE)
	{
	}
	Board::Board(GameConfig cfg) :
			board_data(cfg.rows, cfg.cols),
			game_rules(cfg.rules),
			sign_to_move(Sign::CROSS)
	{
	}
	Board::Board(const std::string &str, Sign signToMove, GameRules rules) :
			board_data(board_from_string(str)),
			game_rules(rules),
			sign_to_move(signToMove)
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
		assert(move.sign == signToMove());
		assert(board_data.at(move.row, move.col) == Sign::NONE);

		board_data.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(move.sign);
	}
	void Board::undoMove(Move move) noexcept
	{
		assert(move.sign == invertSign(signToMove()));
		assert(board_data.at(move.row, move.col) == move.sign);

		board_data.at(move.row, move.col) = Sign::NONE;
		sign_to_move = move.sign;
	}
	void Board::clear() noexcept
	{
		board_data.clear();
	}

	bool Board::isValid() const noexcept
	{
		if (sign_to_move != Sign::CROSS and sign_to_move != Sign::CIRCLE)
			return false;
		int cross_count = std::count(board_data.begin(), board_data.end(), Sign::CROSS);
		int circle_count = std::count(board_data.begin(), board_data.end(), Sign::CIRCLE);

		if (sign_to_move == Sign::CROSS)
			return cross_count == circle_count;
		else
			return cross_count == circle_count + 1;
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
	GameOutcome Board::getOutcome(Move lastMove) const noexcept
	{
		return GameOutcome::UNKNOWN; // TODO
	}
	bool Board::isPossibleToGetFrom(const Board &other) const noexcept
	{
		assert(equalSize(*this, other));
		for (int i = 0; i < other.size(); i++)
			if (this->data()[i] != other.data()[i] and other.data()[i] != Sign::NONE)
				return false;
		return true;
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

