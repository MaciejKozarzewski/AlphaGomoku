/*
 * PatternCalculator_v2_v2.cpp
 *
 *  Created on: Oct 3, 2022
 *      Author: maciek
 */

#include <alphagomoku/patterns/PatternCalculator_v2.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
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
	using namespace ag::experimental;

	template<int Pad>
	constexpr uint32_t line_initial_value() noexcept
	{
		return (1u << (2u * Pad)) - 1u; // basically binary 1 x (2*Pad), rest 0
	}

#define BOARD_AT(row, col) board.at(Pad + (row), Pad + (col))
#define FEATURES_AT(row, col) features.at(Pad + (row), Pad + (col))

	template<int Pad>
	void horizontal(matrix<RawPatternGroup> &features, const matrix<int16_t> &board) noexcept
	{
		constexpr uint32_t shift = 4 * Pad;
		const int number_of_rows = board.rows() - 2 * Pad;
		const int number_of_cols = board.cols() - 2 * Pad;
		for (int row = 0; row < number_of_rows; row++)
		{
			uint32_t line = line_initial_value<Pad>();
			for (int col = 0; col <= Pad; col++)
				line = line | (BOARD_AT(row, col) << ((Pad + col) * 2));
			FEATURES_AT(row, 0).horizontal = line;
			for (int col = 1; col < number_of_cols; col++)
			{
				line = (BOARD_AT(row, col + Pad) << shift) | (line >> 2);
				FEATURES_AT(row, col).horizontal = line;
			}
		}
	}
	template<int Pad>
	void vertical(matrix<RawPatternGroup> &features, const matrix<int16_t> &board) noexcept
	{
		constexpr uint32_t shift = 4 * Pad;
		const int number_of_rows = board.rows() - 2 * Pad;
		const int number_of_cols = board.cols() - 2 * Pad;
		for (int col = 0; col < number_of_cols; col++)
		{
			uint32_t line = line_initial_value<Pad>();
			for (int row = 0; row <= Pad; row++)
				line = line | (BOARD_AT(row, col) << ((Pad + row) * 2));
			FEATURES_AT(0, col).vertical = line;
			for (int row = 1; row < number_of_rows; row++)
			{
				line = (BOARD_AT(row + Pad, col) << shift) | (line >> 2);
				FEATURES_AT(row, col).vertical = line;
			}
		}
	}
	template<int Pad>
	void diagonal(matrix<RawPatternGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		constexpr uint32_t shift = 4 * Pad;
		const int board_size = board.rows() - 2 * Pad;
		for (int i = 0; i < board_size; i++) // lower left half
		{
			uint32_t line = line_initial_value<Pad>();
			for (int j = 0; j <= Pad; j++)
				line = line | (BOARD_AT(i + j, j) << ((Pad + j) * 2));
			FEATURES_AT(i, 0).diagonal = line;
			for (int j = 1; j < board_size - i; j++)
			{
				line = (BOARD_AT(i + j + Pad, j + Pad) << shift) | (line >> 2);
				FEATURES_AT(i + j, j).diagonal = line;
			}
		}

		for (int i = 1; i < board_size; i++) // upper right half
		{
			uint32_t line = line_initial_value<Pad>();
			for (int j = 0; j <= Pad; j++)
				line = line | (BOARD_AT(j, i + j) << ((Pad + j) * 2));
			FEATURES_AT(0, i).diagonal = line;
			for (int j = 1; j < board_size - i; j++)
			{
				line = (BOARD_AT(j + Pad, i + j + Pad) << shift) | (line >> 2);
				FEATURES_AT(j, i + j).diagonal = line;
			}
		}
	}
	template<int Pad>
	void antidiagonal(matrix<RawPatternGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		constexpr uint32_t shift = 4 * Pad;
		const int board_size = board.rows() - 2 * Pad;
		for (int i = 0; i < board_size; i++) // upper left half
		{
			uint32_t line = line_initial_value<Pad>();
			for (int j = 0; j <= Pad; j++)
				line = line | (BOARD_AT(j, i - j) << ((Pad + j) * 2));
			FEATURES_AT(0, i).antidiagonal = line;
			for (int j = 1; j <= i; j++)
			{
				line = (BOARD_AT(j + Pad, i - j - Pad) << shift) | (line >> 2);
				FEATURES_AT(j, i - j).antidiagonal = line;
			}
		}

		for (int i = 0; i < board_size; i++) // lower right half
		{
			uint32_t line = line_initial_value<Pad>();
			for (int j = 0; j <= Pad; j++)
				line = line | (BOARD_AT(i + j, board_size - 1 - j) << ((Pad + j) * 2));
			FEATURES_AT(i, board_size - 1).antidiagonal = line;
			for (int j = 1; j < board_size - i; j++)
			{
				line = (BOARD_AT(i + j + Pad, board_size - 1 - j - Pad) << shift) | (line >> 2);
				FEATURES_AT(i + j, board_size - 1 - j).antidiagonal = line;
			}
		}
	}

	template<int Pad>
	void add_move(ag::matrix<RawPatternGroup> &features, Move move) noexcept
	{
		assert(features.isSquare());
		assert(move.sign != ag::Sign::NONE);
		uint32_t mask1 = static_cast<uint32_t>(move.sign) << (4 * Pad);
		for (int i = -Pad; i <= Pad; i++)
		{
			FEATURES_AT(move.row, move.col + i).horizontal |= mask1;
			FEATURES_AT(move.row + i, move.col).vertical |= mask1;
			FEATURES_AT(move.row + i, move.col + i).diagonal |= mask1;
			FEATURES_AT(move.row + i, move.col - i).antidiagonal |= mask1;
			mask1 = mask1 >> 2;
		}
	}
	template<int Pad>
	void undo_move(ag::matrix<RawPatternGroup> &features, Move move) noexcept
	{
		assert(features.isSquare());
		assert(move.sign != ag::Sign::NONE);
		uint32_t mask1 = ~(3 << (4 * Pad));
		for (int i = -Pad; i <= Pad; i++)
		{
			FEATURES_AT(move.row, move.col + i).horizontal &= mask1;
			FEATURES_AT(move.row + i, move.col).vertical &= mask1;
			FEATURES_AT(move.row + i, move.col + i).diagonal &= mask1;
			FEATURES_AT(move.row + i, move.col - i).antidiagonal &= mask1;
			mask1 = (3 << 30) | (mask1 >> 2);
		}
	}

	template<int Pad>
	int move_value(const ag::matrix<int16_t> &board, Move move, const std::array<int, 4> &values) noexcept
	{
		assert(board.isSquare());
		int result = 0;
		for (int i = -Pad; i <= Pad; i++)
		{
			result += values[BOARD_AT(move.row, move.col + i)];
			result += values[BOARD_AT(move.row + i, move.col)];
			result += values[BOARD_AT(move.row + i, move.col + i)];
			result += values[BOARD_AT(move.row + i, move.col - i)];
		}
		return result;
	}

	/*
	 * \brief Checks if placing cross in some spot creates straight four for cross
	 */
	template<int Pad>
	bool is_straight_four(const matrix<int16_t> &board, Move move, Direction direction) noexcept
	{
		assert(BOARD_AT(move.row, move.col) == static_cast<int16_t>(Sign::NONE));
		assert(move.sign == Sign::CROSS);
		uint32_t result = static_cast<uint32_t>(Sign::CROSS) << (2 * Pad);
		uint32_t shift = 0;
		switch (direction)
		{
			case HORIZONTAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (BOARD_AT(move.row, move.col + i) << shift);
				break;
			case VERTICAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col) << shift);
				break;
			case DIAGONAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col + i) << shift);
				break;
			case ANTIDIAGONAL:
				for (int i = -Pad; i <= Pad; i++, shift += 2)
					result |= (BOARD_AT(move.row + i, move.col - i) << shift);
				break;
		}
		for (int i = 0; i < (2 * Pad + 1 - 4); i++, result /= 4)
			if ((result & 255u) == 85u)
				return true;
		return false;
	}

#undef BOARD_AT
#undef FEATURES_AT
}

namespace ag::experimental
{
	PatternCalculator_v2::PatternCalculator_v2(GameConfig gameConfig) :
			game_config(gameConfig),
			pad((Pattern::length(gameConfig.rules) - 1) / 2),
			legal_moves_mask(gameConfig.rows, gameConfig.cols),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			raw_features(internal_board.rows(), internal_board.cols()),
			feature_types(gameConfig.rows, gameConfig.cols),
			threat_types(gameConfig.rows, gameConfig.cols),
			pattern_table(&PatternTable_v2::get(gameConfig.rules)),
			threat_table(&ThreatTable::get(gameConfig.rules)),
			features_init("features init  "),
			features_class("features class "),
			threats_init("threats init   "),
			features_update("features update"),
			threats_update("threats update ")
	{
		internal_board.fill(3);
	}

	void PatternCalculator_v2::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;
		static_assert(sizeof(Sign) == sizeof(int16_t));
		for (int row = 0; row < board.rows(); row++)
			std::memcpy(internal_board.data(pad + row) + pad, board.data(row), sizeof(Sign) * board.cols());

		legal_moves_mask.fill(false);
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
				if (board.at(row, col) == Sign::NONE)
					legal_moves_mask.at(row, col) = true;

		features_init.startTimer();
		calculate_raw_features();
		features_init.stopTimer();

		features_class.startTimer();
		classify_feature_types();
		features_class.stopTimer();

		threats_init.startTimer();
		prepare_threat_lists();
		threats_init.stopTimer();
	}
	void PatternCalculator_v2::addMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == Sign::NONE); // move must be made on empty spot
		assert(move.sign == sign_to_move);
		threats_update.startTimer();
		update_central_spot(move.row, move.col, ADD_MOVE);
		threats_update.pauseTimer();

		internal_board.at(pad + move.row, pad + move.col) = static_cast<int16_t>(move.sign);
		legal_moves_mask.at(move.row, move.col) = false;

		features_update.startTimer();
		add_move<5>(raw_features, move);
		features_update.stopTimer();

		threats_update.resumeTimer();
		update_neighborhood(move.row, move.col);
		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
	}
	void PatternCalculator_v2::undoMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == move.sign); // board must contain the move to be undone
		assert(move.sign == invertSign(sign_to_move));
		internal_board.at(pad + move.row, pad + move.col) = static_cast<int16_t>(Sign::NONE);
		legal_moves_mask.at(move.row, move.col) = true;

		features_update.startTimer();
		undo_move<5>(raw_features, move);
		features_update.stopTimer();

		threats_update.startTimer();
		update_central_spot(move.row, move.col, UNDO_MOVE);
		update_neighborhood(move.row, move.col);
		threats_update.stopTimer();

		sign_to_move = invertSign(sign_to_move);
	}
	int PatternCalculator_v2::getMoveValue(Move move, const std::array<int, 4> &values) const noexcept
	{
		assert(signAt(move.row, move.col) == Sign::NONE); // move must be made on empty spot
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				return move_value<5>(internal_board, move, values);
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				return move_value<5>(internal_board, move, values);
			default:
				return 0;
		}
	}
	bool PatternCalculator_v2::isForbidden(Sign sign, int row, int col) noexcept
	{
		if (game_config.rules == GameRules::RENJU and sign == Sign::CROSS)
		{
			if (signAt(row, col) != Sign::NONE)
				return false; // moves on occupied spots are not considered forbidden (they are simply illegal)

			const Threat threat = getThreatAt(Sign::CROSS, row, col);
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
						const BitMask<uint16_t> defensive_moves = getDefensiveMoves(Sign::CIRCLE, row, col, dir);
						internal_board.at(pad + row, pad + col) = static_cast<int16_t>(Sign::CROSS);
						for (int i = -5; i <= 5; i++)
						{
							const int x = row + i * get_row_step(dir);
							const int y = col + i * get_col_step(dir);
							if (i != 0 and defensive_moves.get(5 + i) == true and is_straight_four<5>(internal_board, Move(Sign::CROSS, x, y), dir))
							{ // minor optimization as 'is_straight_four' works without adding new move to the pattern calculator
								internal_board.at(pad + row, pad + col) = static_cast<int16_t>(Sign::NONE);
								addMove(Move(row, col, Sign::CROSS));
								const bool is_forbidden = isForbidden(sign, x, y);
								undoMove(Move(row, col, Sign::CROSS));
								internal_board.at(pad + row, pad + col) = static_cast<int16_t>(Sign::CROSS);

								if (not is_forbidden)
								{
									open3_count++;
									break;
								}
							}
						}
						internal_board.at(pad + row, pad + col) = static_cast<int16_t>(Sign::NONE);
					}
				if (open3_count >= 2)
					return true;
			}
			return false;
		}
		return false;
	}

	void PatternCalculator_v2::printRawFeature(int row, int col) const
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

			uint32_t line = getRawFeatureAt(row, col, static_cast<Direction>(i));
			for (int j = 0; j < 2 * pad + 1; j++)
			{
				if (j == pad)
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
				if (j == pad)
					std::cout << ' ';
				line = line / 4;
			}
			std::cout << " : (" << getRawFeatureAt(row, col, static_cast<Direction>(i)) << ") : ";
			for (int s = 1; s <= 2; s++)
			{
				if (s == 2)
					std::cout << " : " << toString(getPatternTypeAt(static_cast<Sign>(s), row, col, static_cast<Direction>(i)));
			}
			std::cout << '\n';
		}
	}
	void PatternCalculator_v2::printThreat(int row, int col) const
	{
		std::cout << "Threat for cross at  (" << row << ", " << col << ") = " << toString(threat_types.at(row, col).forCross()) << '\n';
		std::cout << "Threat for circle at (" << row << ", " << col << ") = " << toString(threat_types.at(row, col).forCircle()) << '\n';
	}
	void PatternCalculator_v2::printAllThreats() const
	{
		std::cout << "Threats-for-cross-----------\n";
		cross_threats.print();
		std::cout << "Threats-for-circle----------\n";
		circle_threats.print();
	}
	void PatternCalculator_v2::print(Move lastMove) const
	{
		for (int i = 0; i < internal_board.rows(); i++)
		{
			for (int j = 0; j < internal_board.cols(); j++)
			{
				if (lastMove.sign != Sign::NONE)
				{
					if ((i - pad) == lastMove.row && (j - pad) == lastMove.col)
						std::cout << '>';
					else
					{
						if ((i - pad) == lastMove.row && (j - pad) == lastMove.col + 1)
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
				if ((i - pad) == lastMove.row && internal_board.cols() - 1 == lastMove.col)
					std::cout << '<';
				else
					std::cout << ' ';
			}
			std::cout << '\n';
		}
		std::cout << std::endl;
	}
	void PatternCalculator_v2::print_stats() const
	{
		std::cout << features_init.toString() << '\n';
		std::cout << features_class.toString() << '\n';
		std::cout << threats_init.toString() << '\n';
		std::cout << features_update.toString() << '\n';
		std::cout << threats_update.toString() << '\n';
		std::cout << neighborhood_updates << " updates\n";
	}
	/*
	 * private
	 */
	void PatternCalculator_v2::calculate_raw_features() noexcept
	{
		horizontal<5>(raw_features, internal_board);
		vertical<5>(raw_features, internal_board);
		diagonal<5>(raw_features, internal_board);
		antidiagonal<5>(raw_features, internal_board);
	}
	void PatternCalculator_v2::classify_feature_types() noexcept
	{
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternEncoding_v2 tmp = pattern_table->getPatternData(getRawFeatureAt(row, col, dir));
						feature_types.at(row, col).for_cross[dir] = tmp.forCross();
						feature_types.at(row, col).for_circle[dir] = tmp.forCircle();
					}
				}
				else
					feature_types.at(row, col) = PatternTypeGroup();
	}
	void PatternCalculator_v2::prepare_threat_lists()
	{
		cross_threats.clear();
		circle_threats.clear();
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					const Threat threat = threat_table->getThreat(feature_types.at(row, col));
					threat_types.at(row, col) = threat;

					cross_threats.add(threat.forCross(), Move(row, col));
					circle_threats.add(threat.forCircle(), Move(row, col));
				}
				else
					threat_types.at(row, col) = Threat();
	}

	void PatternCalculator_v2::update_central_spot(int row, int col, UpdateMode mode) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);
		assert(signAt(row, col) == Sign::NONE);

		for (Direction dir = 0; dir < 4; dir++)
			central_spot_liberties[dir] = pattern_table->getLiberties(getRawFeatureAt(row, col, dir));

		if (mode == ADD_MOVE)
		{ // a stone was added
			const Threat old_threat = threat_types.at(row, col);
			cross_threats.remove(old_threat.forCross(), Move(row, col));
			circle_threats.remove(old_threat.forCircle(), Move(row, col));

			// since this spot is now occupied, there are no threats nor defensive moves
			feature_types.at(row, col) = PatternTypeGroup();
			threat_types.at(row, col) = Threat();
		}
		else
		{ // a stone was removed
			assert(mode == UNDO_MOVE);
			for (Direction dir = 0; dir < 4; dir++)
			{
				const PatternEncoding_v2 tmp = pattern_table->getPatternData(getRawFeatureAt(row, col, dir));
				feature_types.at(row, col).for_cross[dir] = tmp.forCross();
				feature_types.at(row, col).for_circle[dir] = tmp.forCircle();
			}
			const Threat new_threat = threat_table->getThreat(feature_types.at(row, col));

			threat_types.at(row, col) = new_threat; // since the spot is now empty, new threats might have been created
			cross_threats.add(new_threat.forCross(), Move(row, col));
			circle_threats.add(new_threat.forCircle(), Move(row, col));
		}
	}
	void PatternCalculator_v2::update_neighborhood(int row, int col) noexcept
	{
		for (int i = -pad; i <= pad; i++)
			if (i != 0) // central spot has special update procedure
			{
				if (central_spot_liberties[HORIZONTAL].get(pad + i))
					update_feature_types_and_threats(row, col + i, HORIZONTAL);
				if (central_spot_liberties[VERTICAL].get(pad + i))
					update_feature_types_and_threats(row + i, col, VERTICAL);
				if (central_spot_liberties[DIAGONAL].get(pad + i))
					update_feature_types_and_threats(row + i, col + i, DIAGONAL);
				if (central_spot_liberties[ANTIDIAGONAL].get(pad + i))
					update_feature_types_and_threats(row + i, col - i, ANTIDIAGONAL);
			}
	}
	void PatternCalculator_v2::update_feature_types_and_threats(int row, int col, Direction direction) noexcept
	{
		neighborhood_updates++;
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		// find new feature type and update appropriate direction in the table
		const PatternEncoding_v2 new_feature_type = pattern_table->getPatternData(getRawFeatureAt(row, col, direction));
		feature_types.at(row, col).for_cross[direction] = new_feature_type.forCross();
		feature_types.at(row, col).for_circle[direction] = new_feature_type.forCircle();

		const Threat old_threat = threat_types.at(row, col); // check what was the best threat at (row, col) before updating
		const Threat new_threat = threat_table->getThreat(feature_types.at(row, col)); // find new threat type according to the newly updated feature types
		threat_types.at(row, col) = new_threat;

		// update list of threats for cross and circle (if necessary)
		if (old_threat.forCross() != new_threat.forCross())
		{
			cross_threats.remove(old_threat.forCross(), Move(row, col));
			cross_threats.add(new_threat.forCross(), Move(row, col));
		}
		if (old_threat.forCircle() != new_threat.forCircle())
		{
			circle_threats.remove(old_threat.forCircle(), Move(row, col));
			circle_threats.add(new_threat.forCircle(), Move(row, col));
		}
	}

} /* namespace ag */
