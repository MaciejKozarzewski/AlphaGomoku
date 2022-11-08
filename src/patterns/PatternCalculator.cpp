/*
 * PatternCalculator.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/patterns/RawPatterns.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cassert>
#include <bitset>

namespace
{
	using namespace ag;

#define BOARD_AT(row, col) board.at(Pad + (row), Pad + (col))
	/*
	 * \brief Checks if placing cross in some spot creates straight four for cross
	 */
	template<int Pad>
	bool is_straight_four(const matrix<int16_t> &board, Move move, Direction direction) noexcept
	{
		assert(BOARD_AT(move.row, move.col) == static_cast<int16_t>(Sign::NONE));
		assert(move.sign == Sign::CROSS);
		uint32_t result = static_cast<uint32_t>(Sign::CROSS) << 10;
		uint32_t shift = 0;
		switch (direction)
		{
			case HORIZONTAL:
				for (int i = -5; i <= 5; i++, shift += 2)
					result |= (BOARD_AT(move.row, move.col + i) << shift);
				break;
			case VERTICAL:
				for (int i = -5; i <= 5; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col) << shift);
				break;
			case DIAGONAL:
				for (int i = -5; i <= 5; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col + i) << shift);
				break;
			case ANTIDIAGONAL:
				for (int i = -5; i <= 5; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col - i) << shift);
				break;
		}
		for (int i = 0; i < 7; i++, result /= 4)
			if ((result & 255u) == 85u)
				return true;
		return false;
	}
#undef BOARD_AT
}

namespace ag
{
	PatternCalculator::PatternCalculator(GameConfig gameConfig) :
			game_config(gameConfig),
			legal_moves_mask(gameConfig.rows, gameConfig.cols),
			internal_board(gameConfig.rows + 2 * extended_padding, gameConfig.cols + 2 * extended_padding),
			raw_patterns(gameConfig.rows, gameConfig.cols),
			pattern_types(gameConfig.rows, gameConfig.cols),
			threat_types(gameConfig.rows, gameConfig.cols),
			pattern_table(&PatternTable::get(gameConfig.rules)),
			threat_table(&ThreatTable::get(gameConfig.rules)),
			defensive_move_table(&DefensiveMoveTable::get(gameConfig.rules)),
			features_init("features init  "),
			features_class("features class "),
			threats_init("threats init   "),
			features_update("features update"),
			threats_update("threats update ")
	{
		internal_board.fill(3);
		changed_threats.reserve(game_config.rows * game_config.cols);
	}

	void PatternCalculator::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(board.rows() + 2 * extended_padding == internal_board.rows());
		assert(board.cols() + 2 * extended_padding == internal_board.cols());

		changed_moves = Change<Sign>();

		sign_to_move = signToMove;
		static_assert(sizeof(Sign) == sizeof(int16_t));
		for (int row = 0; row < board.rows(); row++)
			std::memcpy(internal_board.data(extended_padding + row) + extended_padding, board.data(row), sizeof(Sign) * board.cols());

		current_depth = 0;
		legal_moves_mask.fill(false);
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
				if (board.at(row, col) == Sign::NONE)
					legal_moves_mask.at(row, col) = true;
				else
					current_depth++;

//		features_init.startTimer();
		raw_patterns.set(internal_board);
//		features_init.stopTimer();

//		features_class.startTimer();
		classify_feature_types();
//		features_class.stopTimer();

//		threats_init.startTimer();
		prepare_threat_lists();
//		threats_init.stopTimer();
	}
	void PatternCalculator::addMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == Sign::NONE); // move must be made on empty spot
		changed_threats.clear();
		changed_moves = Change<Sign>(Sign::NONE, move.sign, move.location());

		internal_board.at(extended_padding + move.row, extended_padding + move.col) = static_cast<int16_t>(move.sign);
		legal_moves_mask.at(move.row, move.col) = false;

//		features_update.startTimer();
		raw_patterns.addMove(move);
//		features_update.stopTimer();

//		threats_update.startTimer();
		update_central_spot<ADD_MOVE>(move.row, move.col, move.sign);
		update_neighborhood(move.row, move.col);
//		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
		current_depth++;
	}
	void PatternCalculator::undoMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == move.sign); // board must contain the move to be undone
		changed_threats.clear();
		changed_moves = Change<Sign>(move.sign, Sign::NONE, move.location());

		internal_board.at(extended_padding + move.row, extended_padding + move.col) = static_cast<int16_t>(Sign::NONE);
		legal_moves_mask.at(move.row, move.col) = true;

//		features_update.startTimer();
		raw_patterns.undoMove(move);
//		features_update.stopTimer();

//		threats_update.startTimer();
		update_central_spot<UNDO_MOVE>(move.row, move.col, move.sign);
		update_neighborhood(move.row, move.col);
//		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
		current_depth--;
		assert(current_depth >= 0);
	}
	bool PatternCalculator::isForbidden(Sign sign, int row, int col) noexcept
	{
		if (game_config.rules == GameRules::RENJU and sign == Sign::CROSS)
		{
			if (signAt(row, col) != Sign::NONE)
				return false; // moves on occupied spots are not considered forbidden (they are simply illegal)

			const ThreatEncoding threat = getThreatAt(Sign::CROSS, row, col);
			if (threat.forCross() == ThreatType::OVERLINE)
				return true;
			if (threat.forCross() == ThreatType::FORK_4x4)
				return true;
			if (threat.forCross() == ThreatType::FORK_3x3)
			{
				int open3_count = 0;
				for (Direction dir = 0; dir < 4; dir++)
					if (getPatternTypeAt(Sign::CROSS, row, col, dir) == PatternType::OPEN_3)
					{
						const BitMask1D<uint16_t> promotion_moves = getOpenThreePromotionMoves(getRawPatternAt(row, col, dir));
						internal_board.at(extended_padding + row, extended_padding + col) = static_cast<int16_t>(Sign::CROSS);
						for (int i = -padding; i <= padding; i++)
						{
							const int x = row + i * get_row_step(dir);
							const int y = col + i * get_col_step(dir);
							if (promotion_moves[padding + i] == true and signAt(x, y) == Sign::NONE
									and is_straight_four<extended_padding>(internal_board, Move(Sign::CROSS, x, y), dir))
							{ // minor optimization as 'is_straight_four' works without adding new move to the pattern calculator
								internal_board.at(extended_padding + row, extended_padding + col) = static_cast<int16_t>(Sign::NONE);
								addMove(Move(row, col, Sign::CROSS));
								const bool is_forbidden = isForbidden(sign, x, y);
								undoMove(Move(row, col, Sign::CROSS));
								internal_board.at(extended_padding + row, extended_padding + col) = static_cast<int16_t>(Sign::CROSS);

								if (not is_forbidden)
								{
									open3_count++;
									break;
								}
							}
						}
						internal_board.at(extended_padding + row, extended_padding + col) = static_cast<int16_t>(Sign::NONE);
					}
				if (open3_count >= 2)
					return true;
			}
			return false;
		}
		return false;
	}

	void PatternCalculator::printRawFeature(int row, int col) const
	{
		std::cout << "Features at (" << row << ", " << col << ")\n";
		for (int i = 0; i < 4; i++)
		{
			switch (i)
			{
				case 0:
					std::cout << "horizontal   ";
					break;
				case 1:
					std::cout << "vertical     ";
					break;
				case 2:
					std::cout << "diagonal     ";
					break;
				case 3:
					std::cout << "antidiagonal ";
					break;
			}

			uint32_t line = getRawPatternAt(row, col, static_cast<Direction>(i));
			for (int j = 0; j < 2 * padding + 1; j++)
			{
				if (j == padding)
					std::cout << ' ';
				switch (line % 4)
				{
					case 0:
						std::cout << '_';
						break;
					case 1:
						std::cout << 'X';
						break;
					case 2:
						std::cout << 'O';
						break;
					case 3:
						std::cout << '|';
						break;
				}
				if (j == padding)
					std::cout << ' ';
				line = line / 4;
			}
			std::cout << " : (" << getRawPatternAt(row, col, static_cast<Direction>(i)) << ") : ";
			std::cout << toString(getPatternTypeAt(Sign::CROSS, row, col, static_cast<Direction>(i))) << " : ";
			std::cout << toString(getPatternTypeAt(Sign::CIRCLE, row, col, static_cast<Direction>(i))) << '\n';
		}
	}
	void PatternCalculator::printThreat(int row, int col) const
	{
		std::cout << "Threat for cross at  (" << row << ", " << col << ") = " << toString(threat_types.at(row, col).forCross()) << '\n';
		std::cout << "Threat for circle at (" << row << ", " << col << ") = " << toString(threat_types.at(row, col).forCircle()) << '\n';
	}
	void PatternCalculator::printAllThreats() const
	{
		std::cout << "Threats-for-cross-----------\n";
		cross_threats.print();
		std::cout << "Threats-for-circle----------\n";
		circle_threats.print();
	}
	void PatternCalculator::print(Move lastMove) const
	{
		for (int i = 0; i < internal_board.rows(); i++)
		{
			for (int j = 0; j < internal_board.cols(); j++)
			{
				if (lastMove.sign != Sign::NONE)
				{
					if ((i - padding) == lastMove.row && (j - padding) == lastMove.col)
						std::cout << '>';
					else
					{
						if ((i - padding) == lastMove.row && (j - padding) == lastMove.col + 1)
							std::cout << '<';
						else
							std::cout << ' ';
					}
				}
				else
					std::cout << ' ';
				switch (internal_board.at(i, j))
				{
					case 0:
						std::cout << "_";
						break;
					case 1:
						std::cout << "X";
						break;
					case 2:
						std::cout << "O";
						break;
					case 3:
						std::cout << "|";
						break;
				}
			}
			if (lastMove.sign != Sign::NONE)
			{
				if ((i - padding) == lastMove.row && internal_board.cols() - 1 == lastMove.col)
					std::cout << '<';
				else
					std::cout << ' ';
			}
			std::cout << '\n';
		}
		std::cout << std::endl;
	}
	void PatternCalculator::print_stats() const
	{
		std::cout << features_init.toString() << '\n';
		std::cout << features_class.toString() << '\n';
		std::cout << threats_init.toString() << '\n';
		std::cout << features_update.toString() << '\n';
		std::cout << threats_update.toString() << '\n';
	}
	/*
	 * private
	 */
	void PatternCalculator::classify_feature_types() noexcept
	{
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternEncoding tmp = pattern_table->getPatternType(getRawPatternAt(row, col, dir));
						pattern_types.at(row, col).for_cross[dir] = tmp.forCross();
						pattern_types.at(row, col).for_circle[dir] = tmp.forCircle();
					}
				}
				else
					pattern_types.at(row, col) = TwoPlayerGroup<DirectionGroup<PatternType>>();
	}
	void PatternCalculator::prepare_threat_lists()
	{
		cross_threats.clear();
		circle_threats.clear();
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					const ThreatEncoding threat = threat_table->getThreat(pattern_types.at(row, col));
					threat_types.at(row, col) = threat;

					cross_threats.add(threat.forCross(), Location(row, col));
					circle_threats.add(threat.forCircle(), Location(row, col));
				}
				else
					threat_types.at(row, col) = ThreatEncoding();
	}

	template<UpdateMode Mode>
	void PatternCalculator::update_central_spot(int row, int col, Sign s) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		for (Direction dir = 0; dir < 4; dir++)
			update_mask[dir] = pattern_table->getUpdateMask(getRawPatternAt(row, col, dir), s);

		if (Mode == ADD_MOVE)
		{ // a stone was added
			assert(signAt(row, col) == s);
			const ThreatEncoding old_threat = threat_types.at(row, col);
			cross_threats.remove(old_threat.forCross(), Location(row, col));
			circle_threats.remove(old_threat.forCircle(), Location(row, col));

			// since this spot is now occupied, there are no threats nor defensive moves
			pattern_types.at(row, col) = TwoPlayerGroup<DirectionGroup<PatternType>>();
			threat_types.at(row, col) = ThreatEncoding();

			changed_threats.emplace_back(old_threat, ThreatEncoding(), Location(row, col));
		}
		else
		{ // a stone was removed
			assert(Mode == UNDO_MOVE);
			assert(signAt(row, col) == Sign::NONE);
			for (Direction dir = 0; dir < 4; dir++)
			{
				const PatternEncoding tmp = pattern_table->getPatternType(getRawPatternAt(row, col, dir));
				pattern_types.at(row, col).for_cross[dir] = tmp.forCross();
				pattern_types.at(row, col).for_circle[dir] = tmp.forCircle();
			}
			const ThreatEncoding new_threat = threat_table->getThreat(pattern_types.at(row, col));

			threat_types.at(row, col) = new_threat; // since the spot is now empty, new threats might have been created
			cross_threats.add(new_threat.forCross(), Location(row, col));
			circle_threats.add(new_threat.forCircle(), Location(row, col));

			changed_threats.emplace_back(ThreatEncoding(), new_threat, Location(row, col));
		}
	}
	void PatternCalculator::update_neighborhood(int row, int col) noexcept
	{
		for (int i = -padding; i <= padding; i++)
			if (i != 0) // central spot has special update procedure
			{
				if (update_mask.horizontal.get(padding + i))
					update_feature_types_and_threats(row, col + i, HORIZONTAL, update_mask.horizontal.get(padding + i));
				if (update_mask.vertical.get(padding + i))
					update_feature_types_and_threats(row + i, col, VERTICAL, update_mask.vertical.get(padding + i));
				if (update_mask.diagonal.get(padding + i))
					update_feature_types_and_threats(row + i, col + i, DIAGONAL, update_mask.diagonal.get(padding + i));
				if (update_mask.antidiagonal.get(padding + i))
					update_feature_types_and_threats(row + i, col - i, ANTIDIAGONAL, update_mask.antidiagonal.get(padding + i));
			}
	}
	void PatternCalculator::update_feature_types_and_threats(int row, int col, Direction direction, int mode) noexcept
	{
		assert(mode != 0); // do not update anything (should not be called)
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		// check what was the best threat at (row, col) before updating
		const ThreatEncoding old_threat = threat_types.at(row, col);
		// find new feature type and update appropriate direction in the table
		const PatternEncoding new_feature_type = pattern_table->getPatternType(getRawPatternAt(row, col, direction));

		if (mode & 1)
		{ // update cross
			pattern_types.at(row, col).for_cross[direction] = new_feature_type.forCross();
			const ThreatType new_threat = threat_table->getThreat<Sign::CROSS>(pattern_types.at(row, col).for_cross);
			threat_types.at(row, col).setForCross(new_threat);

			if (old_threat.forCross() != new_threat)
			{
				cross_threats.remove(old_threat.forCross(), Location(row, col));
				cross_threats.add(new_threat, Location(row, col));
			}
		}
		if (mode & 2)
		{ // update circle
			pattern_types.at(row, col).for_circle[direction] = new_feature_type.forCircle();
			const ThreatType new_threat = threat_table->getThreat<Sign::CIRCLE>(pattern_types.at(row, col).for_circle);
			threat_types.at(row, col).setForCircle(new_threat);

			if (old_threat.forCircle() != new_threat)
			{
				circle_threats.remove(old_threat.forCircle(), Location(row, col));
				circle_threats.add(new_threat, Location(row, col));
			}
		}

		changed_threats.emplace_back(old_threat, threat_types.at(row, col), Location(row, col));
	}

} /* namespace ag */

