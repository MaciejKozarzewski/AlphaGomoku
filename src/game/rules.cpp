/*
 * rules.cpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/solver/PatternCalculator.hpp>

#include <stdexcept>
#include <cassert>

namespace
{
	using namespace ag;
	using namespace ag::experimental;

	constexpr int get_row_step(Direction dir) noexcept
	{
		return (dir == HORIZONTAL) ? 0 : 1;
	}
	constexpr int get_col_step(Direction dir) noexcept
	{
		switch (dir)
		{
			case HORIZONTAL:
				return 1;
			case VERTICAL:
				return 0;
			case DIAGONAL:
				return 1;
			case ANTIDIAGONAL:
				return -1;
			default:
				return 0;
		}
	}
	template<int Pad>
	std::string pattern_to_string(uint32_t pattern)
	{
		std::string result;
		for (int i = 0; i < 2 * Pad + 1; i++, pattern /= 4)
			result += text(static_cast<Sign>(pattern & 3));
		return result;
	}
	uint32_t get(const matrix<Sign> &board, int row, int col) noexcept
	{
		if (row < 0 or row >= board.rows() or col < 0 or col >= board.cols())
			return 3;
		else
			return static_cast<uint32_t>(board.at(row, col));
	}
	template<int Pad>
	RawPatternGroup get_raw_patterns(const matrix<Sign> &board, Move move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != ag::Sign::NONE);
		RawPatternGroup result;
		uint32_t shift = 0;
		for (int i = -Pad; i <= Pad; i++, shift += 2)
		{
			result.horizontal |= (get(board, move.row, move.col + i) << shift);
			result.vertical |= (get(board, move.row + i, move.col) << shift);
			result.diagonal |= (get(board, move.row + i, move.col + i) << shift);
			result.antidiagonal |= (get(board, move.row + i, move.col - i) << shift);
		}
		return result;
	}
	PatternTypeGroup convert_to_patterns(GameRules rules, RawPatternGroup raw)
	{
		PatternTypeGroup result;
		for (Direction dir = 0; dir < 4; dir++)
		{
			const PatternEncoding tmp = PatternTable::get(rules).getPatternData(raw.get(dir));
			result.for_cross[dir] = tmp.forCross();
			result.for_circle[dir] = tmp.forCircle();
		}
		return result;
	}

	bool is_a_win(std::array<PatternType, 4> patterns) noexcept
	{
		for (Direction dir = 0; dir < 4; dir++)
			if (patterns[dir] == PatternType::FIVE)
				return true;
		return false;
	}
	template<int Pad>
	bool is_straight_four(const matrix<Sign> &board, Move move, Direction direction)
	{
		assert(board.isSquare());
		assert(board.at(move.row, move.col) == Sign::NONE);
		assert(move.sign == ag::Sign::CROSS); // this method is intended only for cross (or black) in renju rule
		uint32_t result = static_cast<uint32_t>(move.sign) << (2 * Pad);
		uint32_t shift = 0;
		switch (direction)
		{
			case HORIZONTAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (get(board, move.row, move.col + i) << shift);
				break;
			case VERTICAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (get(board, move.row + i, move.col) << shift);
				break;
			case DIAGONAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (get(board, move.row + i, move.col + i) << shift);
				break;
			case ANTIDIAGONAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (get(board, move.row + i, move.col - i) << shift);
				break;
		}
		for (int i = 0; i < (2 * Pad + 1 - 4); i++, result /= 4)
			if ((result & 255u) == 85u)
				return true;
		return false;
	}
}

namespace ag
{
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
		throw std::logic_error("unknown rule '" + str + "'");
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
		if (str == "UNKNOWN")
			return GameOutcome::UNKNOWN;
		if (str == "DRAW")
			return GameOutcome::DRAW;
		if (str == "CROSS_WIN")
			return GameOutcome::CROSS_WIN;
		if (str == "CIRCLE_WIN")
			return GameOutcome::CIRCLE_WIN;
		throw std::logic_error("unknown outcome '" + str + "'");
	}

	GameOutcome getOutcome_v2(GameRules rules, matrix<Sign> &board, Move lastMove)
	{
		assert(board.isSquare());
		assert(lastMove.sign != Sign::NONE);
		assert(lastMove.row >= 0 && lastMove.row < board.rows());
		assert(lastMove.col >= 0 && lastMove.col < board.cols());
		PatternTypeGroup patterns;
		switch (rules)
		{
			case GameRules::FREESTYLE:
				patterns = convert_to_patterns(rules, get_raw_patterns<4>(board, lastMove));
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				patterns = convert_to_patterns(rules, get_raw_patterns<5>(board, lastMove));
				break;
		}
		if (lastMove.sign == Sign::CROSS)
		{
			if (is_a_win(patterns.for_cross))
				return GameOutcome::CROSS_WIN;
		}
		else
		{
			if (is_a_win(patterns.for_circle))
				return GameOutcome::CIRCLE_WIN;
		}

		if (rules == GameRules::RENJU and isForbidden(board, lastMove))
			return GameOutcome::CIRCLE_WIN;

		if (Board::isFull(board))
			return GameOutcome::DRAW;
		else
			return GameOutcome::UNKNOWN;
	}
	bool isForbidden(matrix<Sign> &board, Move move)
	{
		if (move.sign == Sign::CIRCLE)
			return false; // circle (or white) doesn't have any forbidden moves
		if (board.at(move.row, move.col) != Sign::NONE)
			return false; // moves on occupied spots are not considered forbidden (they are simply illegal)

		const RawPatternGroup raw = get_raw_patterns<5>(board, move);
		PatternTypeGroup patterns = convert_to_patterns(GameRules::RENJU, raw);

		const int number_of_open3 = std::count(patterns.for_cross.begin(), patterns.for_cross.end(), PatternType::OPEN_3);
		if (number_of_open3 >= 2)
			for (Direction dir = 0; dir < 4; dir++)
				if (patterns.for_cross[dir] == PatternType::OPEN_3)
				{
					Board::putMove(board, move);
					const PatternEncoding tmp = PatternTable::get(GameRules::RENJU).getPatternData(raw.get(dir));
					const BitMask<uint16_t> defensive_moves = PatternTable::get(GameRules::RENJU).getDefensiveMoves(tmp, Sign::CIRCLE);
					bool is_really_an_open3 = false;
					for (int i = -5; i <= 5; i++)
					{
						const int x = move.row + i * get_row_step(dir);
						const int y = move.col + i * get_col_step(dir);
						if (defensive_moves.get(5 + i) == true and board.at(x, y) == Sign::NONE) // defensive move will never be outside board
							if (is_straight_four<5>(board, Move(x, y, Sign::CROSS), dir) and not isForbidden(board, Move(x, y, Sign::CROSS)))
							{
								is_really_an_open3 = true;
								break;
							}
					}
					Board::undoMove(board, move);
					if (not is_really_an_open3)
						patterns.for_cross[dir] = PatternType::NONE;
				}

		const Threat threat = ThreatTable::get(GameRules::RENJU).getThreat(patterns);
		if (threat.forCross() == ThreatType::OVERLINE or threat.forCross() == ThreatType::FORK_4x4 or threat.forCross() == ThreatType::FORK_3x3)
			return true;
		else
			return false;
	}

} /* namespace ag */

