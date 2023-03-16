/*
 * Board.cpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <algorithm>
#include <iostream>
#include <x86intrin.h>
#include <cassert>

namespace
{
	using namespace ag;

	std::string to_string(Sign s)
	{
		return " " + text(s);
	}
	std::string to_string(Score s)
	{
		if (s.getEval() == 0 and not s.isProven())
			return "  _ ";
		else
			return s.toFormattedString();
	}
	std::string to_string(int i)
	{
		assert(0 <= i && i <= 1000);
		if (i == 0)
			return "  _ ";
		else
			return sfill(i, 4, false);
	}
	std::string to_string(float f)
	{
		return to_string(static_cast<int>(1000 * f));
	}
	std::string to_string(Value v)
	{
		return to_string(static_cast<int>(1000 * v.getExpectation()));
	}

	std::string pretty_print_top_row(int columns, int spacingLeft, int spacingRight)
	{
		const std::string spaces_left(spacingLeft, ' ');
		const std::string spaces_right(spacingRight, ' ');
		std::string result = "/*        ";
		for (int i = 0; i < columns; i++)
			result += spaces_left + static_cast<char>(static_cast<int>('a') + i) + spaces_right;
		return result + "        */";
	}
	std::string pretty_print_side_info(int row)
	{
		std::string result = "/* ";
		if (row < 10)
			result += ' ';
		return result + std::to_string(row) + " */";
	}

	template<class T>
	std::string board_to_string(const matrix<Sign> &board, const T &other, bool prettyPrint)
	{
		std::string result;
		if (prettyPrint)
			result += pretty_print_top_row(board.cols(), 3, 1) + '\n';
		for (int i = 0; i < board.rows(); i++)
		{
			if (prettyPrint)
				result += pretty_print_side_info(i) + " \"";
			for (int j = 0; j < board.cols(); j++)
			{
				if (board.at(i, j) == Sign::NONE)
					result += " " + to_string(other.at(i, j));
				else
					result += "  " + to_string(board.at(i, j)) + " ";
			}
			if (prettyPrint)
				result += "\" " + pretty_print_side_info(i);
			result += '\n';
		}
		if (prettyPrint)
			result += pretty_print_top_row(board.cols(), 3, 1) + '\n';
		return result;
	}

	struct board_size
	{
			int rows;
			int cols;
	};
	board_size get_board_size_from_string(const std::string &str)
	{
		int height = std::count(str.begin(), str.end(), '\n');		// every line must end with new line character "\n"
		if (height == 0)
			throw std::logic_error("get_board_size_from_string() : height is 0");
		if (str.size() % height != 0)
			throw std::logic_error("get_board_size_from_string() : string size is not divisible by height");
		int width = std::count(str.begin(), str.end(), ' ') / height; // every board spot must contain exactly one leading space
		if (std::count(str.begin(), str.end(), ' ') != height * width)
			throw std::logic_error("get_board_size_from_string() : number of ' ' does not match the board size");
		return board_size { height, width };
	}
}

namespace ag
{
	matrix<Sign> Board::fromString(const std::string &str)
	{
		const board_size size = get_board_size_from_string(str);
		matrix<Sign> result(size.rows, size.cols);
		int counter = 0;
		for (size_t i = 0; i < str.size(); i++)
			switch (str.at(i))
			{
				case ' ':
				case '\n':
					break;
				case '_':
				case '!': // special case sometimes used in tests to indicate point of interest
				case '?': // special case sometimes used in tests to indicate point of interest
					result[counter] = Sign::NONE;
					counter++;
					break;
				case 'X':
					result[counter] = Sign::CROSS;
					counter++;
					break;
				case 'O':
					result[counter] = Sign::CIRCLE;
					counter++;
					break;
				default:
					throw std::logic_error(
							std::string("Board::fromString() : invalid character '") + str.at(i) + "' in row " + std::to_string(counter / size.cols));
			}
		return result;
	}
	matrix<Sign> Board::fromListOfMoves(int rows, int cols, const std::vector<Move> &moves)
	{
		matrix<Sign> result(rows, cols);
		for (size_t i = 0; i < moves.size(); i++)
			Board::putMove(result, moves[i]);
		return result;
	}
	void Board::fromListOfMoves(matrix<Sign> &result, const std::vector<Move> &moves)
	{
		result.clear();
		for (size_t i = 0; i < moves.size(); i++)
			Board::putMove(result, moves[i]);
	}
	std::vector<Move> Board::extractMoves(const std::string &str)
	{
		const board_size size = get_board_size_from_string(str);
		std::vector<Move> result;
		for (size_t i = 0; i < str.size(); i++)
			if (str.at(i) == '!')
				result.push_back(Move(i / size.cols, i % size.cols)); // FIXME this is incorrect
		return result;
	}
	bool Board::isValid(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		if (signToMove != Sign::CROSS and signToMove != Sign::CIRCLE)
			return false;
		const int cross_count = std::count(board.begin(), board.end(), Sign::CROSS);
		const int circle_count = std::count(board.begin(), board.end(), Sign::CIRCLE);

		if (signToMove == Sign::CROSS)
			return cross_count == circle_count;
		else
			return cross_count == circle_count + 1;
	}
	bool Board::isEmpty(const matrix<Sign> &board) noexcept
	{
		return std::all_of(board.begin(), board.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}
	bool Board::isFull(const matrix<Sign> &board) noexcept
	{
		return std::none_of(board.begin(), board.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}

	std::string Board::toString(const matrix<Sign> &board, bool prettyPrint)
	{
		std::string result;
		if (prettyPrint)
			result += pretty_print_top_row(board.cols(), 1, 0) + '\n';
		for (int i = 0; i < board.rows(); i++)
		{
			if (prettyPrint)
				result += pretty_print_side_info(i) + " \"";
			for (int j = 0; j < board.cols(); j++)
				result += to_string(board.at(i, j));
			if (prettyPrint)
				result += "\" " + pretty_print_side_info(i);
			result += '\n';
		}
		if (prettyPrint)
			result += pretty_print_top_row(board.cols(), 1, 0) + '\n';
		return result;
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<Score> &actionScores, bool prettyPrint)
	{
		assert(equalSize(board, actionScores));
		return board_to_string(board, actionScores, prettyPrint);
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<float> &policy, bool prettyPrint)
	{
		assert(equalSize(board, policy));
		return board_to_string(board, policy, prettyPrint);
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<int> &visitCount, bool prettyPrint)
	{
		assert(equalSize(board, visitCount));
		return board_to_string(board, visitCount, prettyPrint);
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<Value> &actionValues, bool prettyPrint)
	{
		assert(equalSize(board, actionValues));
		return board_to_string(board, actionValues, prettyPrint);
	}

	bool Board::isTransitionPossible(const matrix<Sign> &from, const matrix<Sign> &to) noexcept
	{
		assert(equalSize(from, to));
		for (int i = 0; i < from.size(); i++)
			if (to[i] != from[i] and from[i] != Sign::NONE)
				return false;
		return true;
	}
	void Board::putMove(matrix<Sign> &board, Move move) noexcept
	{
		assert(board.at(move.row, move.col) == Sign::NONE);
		board.at(move.row, move.col) = move.sign;
	}
	void Board::undoMove(matrix<Sign> &board, Move move) noexcept
	{
		assert(board.at(move.row, move.col) == move.sign);
		board.at(move.row, move.col) = Sign::NONE;
	}
	Sign Board::getSignAt(const matrix<Sign> &board, Move m) noexcept
	{
		return board.at(m.row, m.col);
	}
	bool Board::isMoveLegal(const matrix<Sign> &board, Move m, GameRules rules) noexcept
	{
		const bool is_spot_empty = board.at(m.row, m.col) == Sign::NONE;
		const bool is_forbidden = (rules == GameRules::RENJU) ? isForbidden(board, m) : false;
		return is_spot_empty and not is_forbidden;
	}

	int Board::numberOfMoves(const matrix<Sign> &board) noexcept
	{
		int result = 0;
		for (int i = 0; i < board.size(); i++)
			result += static_cast<int>(board[i] != Sign::NONE);
		return result;
	}

} /* namespace ag */

