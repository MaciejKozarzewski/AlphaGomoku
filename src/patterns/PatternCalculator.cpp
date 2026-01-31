/*
 * PatternCalculator.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cassert>

namespace ag
{
	PatternCalculator::PatternCalculator(GameConfig gameConfig) :
			game_config(gameConfig),
			legal_moves_mask(gameConfig.rows, gameConfig.cols),
			internal_board(gameConfig.rows, gameConfig.cols),
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
	}

	void PatternCalculator::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(equal_shape(internal_board, board));

		sign_to_move = signToMove;
		internal_board = board;

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
		changed_threats.clear();
		changed_moves = Change<Sign>(Sign::NONE, move.sign, move.location());

		Board::putMove(internal_board, move);
		legal_moves_mask.at(move.row, move.col) = false;

//		features_update.startTimer();
		raw_patterns.addMove(move);
//		features_update.stopTimer();

//		threats_update.startTimer();
		update_around(move.row, move.col, move.sign, ADD_MOVE);
//		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
		current_depth++;
	}
	void PatternCalculator::undoMove(Move move) noexcept
	{
		changed_threats.clear();
		changed_moves = Change<Sign>(move.sign, Sign::NONE, move.location());

		Board::undoMove(internal_board, move);
		legal_moves_mask.at(move.row, move.col) = true;

//		features_update.startTimer();
		raw_patterns.undoMove(move);
//		features_update.stopTimer();

//		threats_update.startTimer();
		update_around(move.row, move.col, move.sign, UNDO_MOVE);
//		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
		current_depth--;
		assert(current_depth >= 0);
	}

	void PatternCalculator::printRawFeature(int row, int col) const
	{
		std::cout << "Features at (" << row << ", " << col << ")\n";
		for (Direction dir = 0; dir < 4; dir++)
		{
			switch (dir)
			{
				case HORIZONTAL:
					std::cout << "horizontal   ";
					break;
				case VERTICAL:
					std::cout << "vertical     ";
					break;
				case DIAGONAL:
					std::cout << "diagonal     ";
					break;
				case ANTIDIAGONAL:
					std::cout << "antidiagonal ";
					break;
			}

			uint32_t line = getExtendedPatternAt(row, col, dir);
			for (int j = 0; j < 2 * extended_padding + 1; j++)
			{
				if (j == extended_padding or j == 1 or j == 2 * extended_padding)
					std::cout << ' ';
				std::cout << text(static_cast<Sign>(line % 4));
				if (j == extended_padding)
					std::cout << ' ';
				line = line / 4;
			}
			std::cout << " : (" << getNormalPatternAt(row, col, dir) << ") : ";
			std::cout << toString(getPatternTypeAt(Sign::CROSS, row, col, dir)) << " : ";
			std::cout << toString(getPatternTypeAt(Sign::CIRCLE, row, col, dir)) << '\n';
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
	void PatternCalculator::printForbiddenMoves()
	{
		std::cout << "Forbidden moves for cross\n";
		for (int i = 0; i < internal_board.rows(); i++)
			for (int j = 0; j < internal_board.cols(); j++)
				if (isForbidden(Sign::CROSS, i, j))
					std::cout << Move(i, j, Sign::CROSS).text() << '\n';
	}
	void PatternCalculator::print(Move lastMove) const
	{
		if (lastMove == Move())
		{
			std::cout << Board::toString(internal_board, true);
			return;
		}
		for (int i = 0; i < internal_board.rows(); i++)
		{
			for (int j = 0; j < internal_board.cols(); j++)
			{
				if (lastMove.sign != Sign::NONE)
				{
					if (i == lastMove.row && j == lastMove.col)
						std::cout << '>';
					else
					{
						if (i == lastMove.row && j == lastMove.col + 1)
							std::cout << '<';
						else
							std::cout << ' ';
					}
				}
				else
					std::cout << ' ';
				std::cout << text(internal_board.at(i, j));
			}
			if (lastMove.sign != Sign::NONE)
			{
				if (i == lastMove.row && internal_board.cols() - 1 == lastMove.col)
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
	bool PatternCalculator::is_3x3_forbidden(Sign sign, int row, int col) noexcept
	{
		int open3_count = 0;
		for (Direction dir = 0; dir < 4; dir++)
			if (getPatternTypeAt(Sign::CROSS, row, col, dir) == PatternType::OPEN_3)
			{
				const BitMask1D<uint16_t> promotion_moves = getOpenThreePromotionMoves(getNormalPatternAt(row, col, dir));
				Board::putMove(internal_board, Move(row, col, Sign::CROSS));
				for (int i = -padding; i <= padding; i++)
					if (promotion_moves[padding + i])
					{
						const Location loc = shiftInDirection(dir, i, Location(row, col));
						if (signAt(loc.row, loc.col) == Sign::NONE
								and RawPatternCalculator::isStraightFourAt(internal_board, Move(Sign::CROSS, loc), dir))
						{ // minor optimization as 'isStraightFourAt' works without adding new move to the pattern calculator
							Board::undoMove(internal_board, Move(row, col, Sign::CROSS));
							addMove(Move(row, col, Sign::CROSS));
							const bool is_forbidden = isForbidden(sign, loc.row, loc.col);
							undoMove(Move(row, col, Sign::CROSS));
							Board::putMove(internal_board, Move(row, col, Sign::CROSS));

							if (not is_forbidden)
							{
								open3_count++;
								break;
							}
						}
					}
				Board::undoMove(internal_board, Move(row, col, Sign::CROSS));
			}
		return open3_count >= 2;
	}
	void PatternCalculator::classify_feature_types() noexcept
	{
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternEncoding tmp = pattern_table->getPatternType(getNormalPatternAt(row, col, dir));
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
	void PatternCalculator::update_around(int row, int col, Sign s, UpdateMode mode) noexcept
	{
		assert(internal_board.isInside(row, col));

		DirectionGroup<UpdateMask> update_mask;
		for (Direction dir = 0; dir < 4; dir++)
			update_mask[dir] = pattern_table->getUpdateMask(getNormalPatternAt(row, col, dir), s);

		if (mode == ADD_MOVE)
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
			assert(mode == UNDO_MOVE);
			assert(signAt(row, col) == Sign::NONE);
			for (Direction dir = 0; dir < 4; dir++)
			{
				const PatternEncoding tmp = pattern_table->getPatternType(getNormalPatternAt(row, col, dir));
				pattern_types.at(row, col).for_cross[dir] = tmp.forCross();
				pattern_types.at(row, col).for_circle[dir] = tmp.forCircle();
			}
			const ThreatEncoding new_threat = threat_table->getThreat(pattern_types.at(row, col));

			threat_types.at(row, col) = new_threat; // since the spot is now empty, new threats might have been created
			cross_threats.add(new_threat.forCross(), Location(row, col));
			circle_threats.add(new_threat.forCircle(), Location(row, col));

			changed_threats.emplace_back(ThreatEncoding(), new_threat, Location(row, col));
		}

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
		assert(mode == 1 || mode == 2 || mode == 3); // mode can be either update CROSS, update CIRCLE or update both
		assert(internal_board.isInside(row, col));

		// check what was the best threat at (row, col) before updating
		const ThreatEncoding old_threat = threat_types.at(row, col);
		// find new feature type and update appropriate direction in the table
		const PatternEncoding new_feature_type = pattern_table->getPatternType(getNormalPatternAt(row, col, direction));

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

