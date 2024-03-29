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
	TwoPlayerGroup<DirectionGroup<PatternType>> convert_to_patterns(GameRules rules, DirectionGroup<NormalPattern> raw)
	{
		TwoPlayerGroup<DirectionGroup<PatternType>> result;
		const PatternTable &table = PatternTable::get(rules);
		for (Direction dir = 0; dir < 4; dir++)
		{
			const PatternEncoding tmp = table.getPatternType(raw[dir]);
			result.for_cross[dir] = tmp.forCross();
			result.for_circle[dir] = tmp.forCircle();
		}
		return result;
	}

	bool is_a_win(DirectionGroup<PatternType> patterns) noexcept
	{
		return patterns.contains(PatternType::FIVE);
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
			case GameRules::CARO5:
				return "CARO5";
			case GameRules::CARO6:
				return "CARO6";
			default:
				return "";
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
		if (str == "CARO5")
			return GameRules::CARO5;
		if (str == "CARO6")
			return GameRules::CARO6;
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

	GameOutcome getOutcome(GameRules rules, const matrix<Sign> &board, Move lastMove, int numberOfMovesForDraw)
	{
		if (not board.isInside(lastMove.row, lastMove.col))
			return GameOutcome::UNKNOWN;
		assert(board.isSquare());
		assert(lastMove.sign != Sign::NONE);
		const DirectionGroup<NormalPattern> raw_patterns = RawPatternCalculator::getPatternsAt<NormalPattern>(board, lastMove);
		const TwoPlayerGroup<DirectionGroup<PatternType>> pattern_types = convert_to_patterns(rules, raw_patterns);
		if (lastMove.sign == Sign::CROSS)
		{
			if (is_a_win(pattern_types.for_cross))
				return GameOutcome::CROSS_WIN;
		}
		else
		{
			if (is_a_win(pattern_types.for_circle))
				return GameOutcome::CIRCLE_WIN;
		}
		if (rules == GameRules::RENJU and isForbidden(board, lastMove))
			return GameOutcome::CIRCLE_WIN;

		const bool is_draw = (numberOfMovesForDraw > 0) ? (Board::numberOfMoves(board) >= numberOfMovesForDraw) : Board::isFull(board);
		return is_draw ? GameOutcome::DRAW : GameOutcome::UNKNOWN;
	}
	bool isForbidden(const matrix<Sign> &board, Move move)
	{
		if (move.sign == Sign::CIRCLE)
			return false; // circle (or white) doesn't have any forbidden moves

		const DirectionGroup<NormalPattern> raw_patterns = RawPatternCalculator::getPatternsAt<NormalPattern>(board, move);
		DirectionGroup<PatternType> pattern_types = convert_to_patterns(GameRules::RENJU, raw_patterns).for_cross;

		ThreatType threat_type = ThreatTable::get(GameRules::RENJU).getThreat<Sign::CROSS>(pattern_types);
		if (threat_type == ThreatType::FORK_3x3)
		{
			matrix<Sign> tmp_board(board);
			tmp_board.at(move.row, move.col) = Sign::NONE; // clear the spot that is being checked, just in case it is already occupied
			for (Direction dir = 0; dir < 4; dir++)
				if (pattern_types[dir] == PatternType::OPEN_3)
				{
					Board::putMove(tmp_board, move);
					const BitMask1D<uint16_t> promotion_moves = getOpenThreePromotionMoves(raw_patterns[dir]);
					bool is_really_an_open3 = false;
					for (int i = -5; i <= 5; i++)
						if (i != 0 and promotion_moves[5 + i])
						{
							const Location loc = shiftInDirection(dir, i, move);
							if (tmp_board.at(loc.row, loc.col) == Sign::NONE) // promotion move will never be outside board
								if (RawPatternCalculator::isStraightFourAt(tmp_board, Move(loc, Sign::CROSS), dir)
										and not isForbidden(tmp_board, Move(loc, Sign::CROSS)))
								{
									is_really_an_open3 = true;
									break;
								}
						}
					Board::undoMove(tmp_board, move);
					if (not is_really_an_open3)
						pattern_types[dir] = PatternType::NONE;
				}
			threat_type = ThreatTable::get(GameRules::RENJU).getThreat<Sign::CROSS>(pattern_types); // recalculating threat as some open threes may have turned out to be fake ones
		}

		if (threat_type == ThreatType::OVERLINE or threat_type == ThreatType::FORK_4x4 or threat_type == ThreatType::FORK_3x3)
			return true;
		else
			return false;
	}

} /* namespace ag */

