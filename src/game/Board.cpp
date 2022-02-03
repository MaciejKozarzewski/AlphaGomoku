/*
 * Board.cpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/rules/freestyle.hpp>
#include <alphagomoku/rules/standard.hpp>
#include <alphagomoku/rules/renju.hpp>
#include <alphagomoku/rules/caro.hpp>

#include <algorithm>
#include <iostream>

namespace
{
	using namespace ag;

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
			if (t < 10)
				return "   " + std::to_string(t);
			else
			{
				if (t < 100)
					return "  " + std::to_string(t);
				else
					return " " + std::to_string(t);
			}
		}
	}
	std::string to_string(Value v)
	{
		return to_string(v.win + 0.5f * v.draw);
	}
	template<class T>
	std::string board_to_string(const matrix<Sign> &board, const T &other)
	{
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
				if (board.at(i, j) == Sign::NONE)
					result += " " + to_string(other.at(i, j));
				else
					result += "  " + to_string(board.at(i, j)) + " ";
			result += '\n';
		}
		return result;
	}
}

namespace ag
{
	matrix<Sign> Board::fromString(const std::string &str)
	{
		int height = std::count(str.begin(), str.end(), '\n'); // every line must end with new line character "\n"
		assert(str.size() % height == 0);
		int width = std::count(str.begin(), str.end(), ' ') / height; // every board spot must contain exactly one leading space

		assert(std::count(str.begin(), str.end(), ' ') == height * width);
		matrix<Sign> result(height, width);
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
	bool Board::isValid(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		if (signToMove != Sign::CROSS and signToMove != Sign::CIRCLE)
			return false;
		int cross_count = std::count(board.begin(), board.end(), Sign::CROSS);
		int circle_count = std::count(board.begin(), board.end(), Sign::CIRCLE);

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
	bool Board::isForbidden(const matrix<Sign> &board, Move move) noexcept
	{
		return false; // TODO
	}
	GameOutcome Board::getOutcome(GameRules rules, const matrix<Sign> &board) noexcept
	{
		switch (rules)
		{
			default:
				return GameOutcome::UNKNOWN;
			case GameRules::FREESTYLE:
				return getOutcomeFreestyle(board);
			case GameRules::STANDARD:
				return getOutcomeStandard(board);
			case GameRules::RENJU:
				return getOutcomeRenju(board);
			case GameRules::CARO:
				return getOutcomeCaro(board);
		}
	}
	GameOutcome Board::getOutcome(GameRules rules, const matrix<Sign> &board, Move lastMove) noexcept
	{
		switch (rules)
		{
			default:
				return GameOutcome::UNKNOWN;
			case GameRules::FREESTYLE:
				return getOutcomeFreestyle(board, lastMove);
			case GameRules::STANDARD:
				return getOutcomeStandard(board, lastMove);
			case GameRules::RENJU:
				return getOutcomeRenju(board, lastMove);
			case GameRules::CARO:
				return getOutcomeCaro(board, lastMove);
		}
	}

	std::string Board::toString(const matrix<Sign> &board)
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
	std::string Board::toString(const matrix<Sign> &board, const matrix<ProvenValue> &pv)
	{
		return board_to_string(board, pv);
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<float> &policy)
	{
		return board_to_string(board, policy);
	}
	std::string Board::toString(const matrix<Sign> &board, const matrix<Value> &actionValues)
	{
		return board_to_string(board, actionValues);
	}

	bool Board::isTransitionPossible(const matrix<Sign> &from, const matrix<Sign> &to) noexcept
	{
		assert(equalSize(from, to));
		for (int i = 0; i < from.size(); i++)
			if (to.data()[i] != from.data()[i] and from.data()[i] != Sign::NONE)
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

	int Board::numberOfMoves(const matrix<Sign> &board) noexcept
	{
		return std::count_if(board.begin(), board.end(), [](Sign s)
		{	return s != Sign::NONE;});
	}

} /* namespace ag */

