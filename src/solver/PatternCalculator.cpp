/*
 * PatternCalculator.cpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/solver/PatternCalculator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <cassert>

namespace
{
	using namespace ag::experimental;

	template<int Pad>
	constexpr uint32_t line_initial_value() noexcept
	{
		return (1u << (2u * Pad)) - 1u; // basically binary 1 x (2*Pad), rest 0
	}

#define BOARD_AT(row, col) board.at(Pad + (row), Pad + (col))
#define FEATURES_AT(row, col) features.at(Pad + (row), Pad + (col))

	template<int Pad>
	void horizontal(ag::matrix<RawPatternGroup> &features, const ag::matrix<int16_t> &board) noexcept
	{
		const uint32_t shift = 4 * Pad;
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
	void vertical(ag::matrix<RawPatternGroup> &features, const ag::matrix<int16_t> &board) noexcept
	{
		const uint32_t shift = 4 * Pad;
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
	void diagonal(ag::matrix<RawPatternGroup> &features, const ag::matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
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
	void antidiagonal(ag::matrix<RawPatternGroup> &features, const ag::matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
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
	void add_move(ag::matrix<RawPatternGroup> &features, ag::Move move) noexcept
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
	void undo_move(ag::matrix<RawPatternGroup> &features, ag::Move move) noexcept
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

#undef BOARD_AT
#undef FEATURES_AT

	constexpr int get_padding(ag::GameRules rules) noexcept
	{
		switch (rules)
		{
			case ag::GameRules::FREESTYLE:
				return 4;
			case ag::GameRules::STANDARD:
			case ag::GameRules::RENJU:
			case ag::GameRules::CARO:
				return 5;
			default:
				return 0;
		}
	}
}

namespace ag::experimental
{
	void ThreatHistogram::print() const
	{
		for (size_t i = 4; i < threats.size(); i++)
			for (size_t j = 0; j < threats[i].size(); j++)
				std::cout << threats[i][j].toString() << " : " << toString(static_cast<ThreatType>(i)) << '\n';
	}

	PatternCalculator::PatternCalculator(GameConfig gameConfig) :
			game_config(gameConfig),
			pad(get_padding(gameConfig.rules)),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			raw_features(internal_board.rows(), internal_board.cols()),
			feature_types(gameConfig.rows, gameConfig.cols),
			defensive_moves(gameConfig.rows, gameConfig.cols),
			threat_types(gameConfig.rows, gameConfig.cols),
			pattern_table(&PatternTable::get(gameConfig.rules)),
			threat_table(&ThreatTable::get(gameConfig.rules)),
			features_init("features init  "),
			features_class("features class "),
			threats_init("threats init   "),
			features_update("features update"),
			threats_update("threats update ")
	{
		internal_board.fill(3);
	}

	void PatternCalculator::setBoard(const matrix<Sign> &board, bool flipColors)
	{
		assert(flipColors == false);
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		static_assert(sizeof(Sign) == sizeof(int16_t));
		for (int row = 0; row < board.rows(); row++)
			std::memcpy(internal_board.data(pad + row) + pad, board.data(row), sizeof(Sign) * board.cols());

		if (flipColors)
		{
			const int16_t tmp_table[4] = { 0, 2, 1, 3 };
			for (int i = 0; i < internal_board.size(); i++)
				internal_board[i] = tmp_table[internal_board[i]];
		}

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
	void PatternCalculator::addMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == Sign::NONE); // move must be made on empty spot
		threats_update.startTimer();
		update_central_spot(move.row, move.col, +1);
		threats_update.pauseTimer();

		internal_board.at(pad + move.row, pad + move.col) = static_cast<int16_t>(move.sign);

		features_update.startTimer();
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				add_move<4>(raw_features, move);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				add_move<5>(raw_features, move);
				break;
			default:
				break;
		}
		features_update.stopTimer();

		threats_update.resumeTimer();
		update_neighborhood(move.row, move.col);
		threats_update.stopTimer();
	}
	void PatternCalculator::undoMove(Move move) noexcept
	{
		assert(signAt(move.row, move.col) == move.sign); // board must contain the move to be undone
		internal_board.at(pad + move.row, pad + move.col) = static_cast<int16_t>(Sign::NONE);

		features_update.startTimer();
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				undo_move<4>(raw_features, move);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				undo_move<5>(raw_features, move);
				break;
			default:
				break;
		}
		features_update.stopTimer();

		threats_update.startTimer();
		update_central_spot(move.row, move.col, -1);
		update_neighborhood(move.row, move.col);
		threats_update.stopTimer();
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
					std::cout << " : ";
				switch (getPatternTypeAt(static_cast<Sign>(s), row, col, static_cast<Direction>(i)))
				{
					case PatternType::NONE:
						std::cout << "NONE";
						break;
					case PatternType::OPEN_3:
						std::cout << "OPEN_3";
						break;
					case PatternType::HALF_OPEN_4:
						std::cout << "HALF_OPEN_4";
						break;
					case PatternType::OPEN_4:
						std::cout << "OPEN_4";
						break;
					case PatternType::DOUBLE_4:
						std::cout << "DOUBLE_4";
						break;
					case PatternType::FIVE:
						std::cout << "FIVE";
						break;
					case PatternType::OVERLINE:
						std::cout << "OVERLINE";
						break;
				}
			}
			std::cout << '\n';
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
	void PatternCalculator::calculate_raw_features() noexcept
	{
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				horizontal<4>(raw_features, internal_board);
				vertical<4>(raw_features, internal_board);
				diagonal<4>(raw_features, internal_board);
				antidiagonal<4>(raw_features, internal_board);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				horizontal<5>(raw_features, internal_board);
				vertical<5>(raw_features, internal_board);
				diagonal<5>(raw_features, internal_board);
				antidiagonal<5>(raw_features, internal_board);
				break;
			default:
				break;
		}
	}
	void PatternCalculator::classify_feature_types() noexcept
	{
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (int dir = 0; dir < 4; dir++)
					{
						const PatternEncoding tmp = pattern_table->getPatternData(getRawFeatureAt(row, col, static_cast<Direction>(dir)));
						feature_types.at(row, col).for_cross[dir] = tmp.forCross();
						feature_types.at(row, col).for_circle[dir] = tmp.forCircle();

						defensive_moves.at(row, col).for_cross[dir] = pattern_table->getDefensiveMoves(tmp, Sign::CROSS);
						defensive_moves.at(row, col).for_circle[dir] = pattern_table->getDefensiveMoves(tmp, Sign::CIRCLE);
					}
				}
				else
				{
					feature_types.at(row, col) = PatternTypeGroup();
					defensive_moves.at(row, col) = DefensiveMoves();
				}
	}
	void PatternCalculator::prepare_threat_lists()
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

	void PatternCalculator::update_central_spot(int row, int col, int mode) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);
		assert(mode == 1 || mode == -1);
		assert(signAt(row, col) == Sign::NONE);

		for (int dir = 0; dir < 4; dir++)
			central_spot_encoding[dir] = pattern_table->getPatternData(getRawFeatureAt(row, col, static_cast<Direction>(dir)));

		if (mode == 1) // a stone was added
		{
			const Threat old_threat = threat_types.at(row, col);
			cross_threats.remove(old_threat.forCross(), Move(row, col));
			circle_threats.remove(old_threat.forCircle(), Move(row, col));

			// since this spot is now occupied, there are no threats nor defensive moves
			feature_types.at(row, col) = PatternTypeGroup();
			defensive_moves.at(row, col) = DefensiveMoves();
			threat_types.at(row, col) = Threat();
		}
		else // a stone was removed
		{
			for (int dir = 0; dir < 4; dir++)
			{
				feature_types.at(row, col).for_cross[dir] = central_spot_encoding[dir].forCross();
				feature_types.at(row, col).for_circle[dir] = central_spot_encoding[dir].forCircle();

				defensive_moves.at(row, col).for_cross[dir] = pattern_table->getDefensiveMoves(central_spot_encoding[dir], Sign::CROSS);
				defensive_moves.at(row, col).for_circle[dir] = pattern_table->getDefensiveMoves(central_spot_encoding[dir], Sign::CIRCLE);
			}
			const Threat new_threat = threat_table->getThreat(feature_types.at(row, col));

			threat_types.at(row, col) = new_threat; // since the spot is now empty, new threats might have been created
			cross_threats.add(new_threat.forCross(), Move(row, col));
			circle_threats.add(new_threat.forCircle(), Move(row, col));
		}
	}
	void PatternCalculator::update_neighborhood(int row, int col) noexcept
	{
		for (int i = -pad; i <= pad; i++)
			if (i != 0) // central spot has special update procedure
			{
				if (central_spot_encoding[HORIZONTAL].mustBeUpdated(pad + i))
					update_feature_types_and_threats(row, col + i, HORIZONTAL);
				if (central_spot_encoding[VERTICAL].mustBeUpdated(pad + i))
					update_feature_types_and_threats(row + i, col, VERTICAL);
				if (central_spot_encoding[DIAGONAL].mustBeUpdated(pad + i))
					update_feature_types_and_threats(row + i, col + i, DIAGONAL);
				if (central_spot_encoding[ANTIDIAGONAL].mustBeUpdated(pad + i))
					update_feature_types_and_threats(row + i, col - i, ANTIDIAGONAL);
			}
	}
	void PatternCalculator::update_feature_types_and_threats(int row, int col, int direction) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		// find new feature type and update appropriate direction in the table
		const PatternEncoding new_feature_type = pattern_table->getPatternData(getRawFeatureAt(row, col, static_cast<Direction>(direction)));
		feature_types.at(row, col).for_cross[direction] = new_feature_type.forCross();
		feature_types.at(row, col).for_circle[direction] = new_feature_type.forCircle();

		const Threat old_threat = threat_types.at(row, col); // check what was the best threat at (row, col) before updating
		const Threat new_threat = threat_table->getThreat(feature_types.at(row, col)); // find new threat type according to the newly updated feature types
		threat_types.at(row, col) = new_threat;

		defensive_moves.at(row, col).for_cross[direction] = pattern_table->getDefensiveMoves(new_feature_type, Sign::CROSS);
		defensive_moves.at(row, col).for_circle[direction] = pattern_table->getDefensiveMoves(new_feature_type, Sign::CIRCLE);

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

