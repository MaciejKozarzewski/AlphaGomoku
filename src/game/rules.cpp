/*
 * rules.cpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/patterns/Pattern.hpp>

#include <stdexcept>
#include <cassert>

namespace
{
	using namespace ag;

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
	DirectionGroup<uint32_t> get_raw_patterns(const matrix<Sign> &board, Move move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != ag::Sign::NONE);
		DirectionGroup<uint32_t> result = { 0u, 0u, 0u, 0u };
		uint32_t shift = 0;
		for (int i = -Pad; i <= Pad; i++, shift += 2)
		{
			result.horizontal |= (get(board, move.row, move.col + i) << shift);
			result.vertical |= (get(board, move.row + i, move.col) << shift);
			result.diagonal |= (get(board, move.row + i, move.col + i) << shift);
			result.antidiagonal |= (get(board, move.row + i, move.col - i) << shift);
		}
		if (board.at(move.row, move.col) != Sign::NONE)
		{ // the patterns should be calculated for empty spot, so we have to correct it now
			const uint32_t mask = (((1u << (2u * Pad)) - 1u) << (2 * Pad + 2)) | ((1u << (2 * Pad)) - 1u);
			result.horizontal &= mask;
			result.vertical &= mask;
			result.diagonal &= mask;
			result.antidiagonal &= mask;
		}
		return result;
	}
	TwoPlayerGroup<PatternType> convert_to_patterns(GameRules rules, DirectionGroup<uint32_t> raw)
	{
		TwoPlayerGroup<PatternType> result;
		for (Direction dir = 0; dir < 4; dir++)
		{
			const PatternEncoding tmp = PatternTable::get(rules).getPatternData(raw[dir]);
			result.for_cross[dir] = tmp.forCross();
			result.for_circle[dir] = tmp.forCircle();
		}
		return result;
	}

	bool is_a_win(DirectionGroup<PatternType> patterns) noexcept
	{
		return patterns.contains(PatternType::FIVE);
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

	GameOutcome getOutcome_v2(GameRules rules, const matrix<Sign> &board, Move lastMove, int numberOfMovesForDraw)
	{
		assert(board.at(lastMove.row, lastMove.col) != lastMove.sign); // a move must already has been made on board
		if (lastMove.row < 0 or lastMove.row >= board.rows() or lastMove.col < 0 or lastMove.col >= board.cols())
			return GameOutcome::UNKNOWN;
		assert(board.isSquare());
		assert(lastMove.sign != Sign::NONE);
		const TwoPlayerGroup<PatternType> patterns = convert_to_patterns(rules, get_raw_patterns<5>(board, lastMove));
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

		const bool is_draw = (numberOfMovesForDraw > 0) ? (Board::numberOfMoves(board) >= numberOfMovesForDraw) : Board::isFull(board);
		return is_draw ? GameOutcome::DRAW : GameOutcome::UNKNOWN;
	}
	bool isForbidden(const matrix<Sign> &board, Move move)
	{
		constexpr int Pad = 5;

		if (move.sign == Sign::CIRCLE)
			return false; // circle (or white) doesn't have any forbidden moves

		const DirectionGroup<uint32_t> raw = get_raw_patterns<Pad>(board, move);
		TwoPlayerGroup<PatternType> patterns = convert_to_patterns(GameRules::RENJU, raw);

		ThreatEncoding threat = ThreatTable::get(GameRules::RENJU).getThreat(patterns);
		if (threat.forCross() == ThreatType::FORK_3x3)
		{
			matrix<Sign> tmp_board(board);
			for (Direction dir = 0; dir < 4; dir++)
				if (patterns.for_cross[dir] == PatternType::OPEN_3)
				{
					Board::putMove(tmp_board, move);
					const BitMask1D<uint16_t> promotion_moves = getOpenThreePromotionMoves(raw[dir]);
					bool is_really_an_open3 = false;
					for (int i = -Pad; i <= Pad; i++)
						if (i != 0)
						{
							const int x = move.row + i * get_row_step(dir);
							const int y = move.col + i * get_col_step(dir);
							if (promotion_moves[Pad + i] == true and tmp_board.at(x, y) == Sign::NONE) // defensive move will never be outside board
								if (is_straight_four<Pad>(tmp_board, Move(x, y, Sign::CROSS), dir)
										and not isForbidden(tmp_board, Move(x, y, Sign::CROSS)))
								{
									is_really_an_open3 = true;
									break;
								}
						}
					Board::undoMove(tmp_board, move);
					if (not is_really_an_open3)
						patterns.for_cross[dir] = PatternType::NONE;
				}
			threat = ThreatTable::get(GameRules::RENJU).getThreat(patterns); // recalculating threat as some open threes may have turned out to be fake ones
		}

		if (threat.forCross() == ThreatType::OVERLINE or threat.forCross() == ThreatType::FORK_4x4 or threat.forCross() == ThreatType::FORK_3x3)
			return true;
		else
			return false;
	}

} /* namespace ag */

