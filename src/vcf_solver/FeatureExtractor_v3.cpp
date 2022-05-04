/*
 * FeatureExtractor_v3.cpp
 *
 *  Created on: Apr 11, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureExtractor_v3.hpp>
#include <alphagomoku/vcf_solver/FeatureTable_v3.hpp>
#include <alphagomoku/vcf_solver/ThreatTable.hpp>
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
	using namespace ag;

	/* direction */
	const int HORIZONTAL = 0;
	const int VERTICAL = 1;
	const int DIAGONAL = 2;
	const int ANTIDIAGONAL = 3;

	template<int Pad>
	constexpr uint32_t line_initial_value() noexcept
	{
		return (1u << (2u * Pad)) - 1u; // basically binary 1 x (2*Pad), rest 0
	}

#define BOARD_AT(row, col) board.at(Pad + (row), Pad + (col))
#define FEATURES_AT(row, col) features.at(Pad + (row), Pad + (col))

	template<int Pad>
	void horizontal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
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
	void vertical(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
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
	void diagonal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
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
	void antidiagonal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
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
	void add_move(matrix<FeatureGroup> &features, Move move) noexcept
	{
		assert(features.isSquare());
		assert(move.sign != Sign::NONE);
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
	void undo_move(matrix<FeatureGroup> &features, Move move) noexcept
	{
		assert(features.isSquare());
		assert(move.sign != Sign::NONE);
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

	constexpr int get_padding(GameRules rules) noexcept
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return 4;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				return 5;
			default:
				return 0;
		}
	}
	const FeatureTable_v3& get_feature_table(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static FeatureTable_v3 table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static FeatureTable_v3 table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static FeatureTable_v3 table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static FeatureTable_v3 table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}
	const ThreatTable& get_threat_table(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static ThreatTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static ThreatTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static ThreatTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static ThreatTable table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}
}

namespace ag
{
	void ThreatHistogram::print() const
	{
		for (size_t i = 1; i < threats.size(); i++)
			for (size_t j = 0; j < threats[i].size(); j++)
				std::cout << threats[i][j].toString() << " : " << ag::toString(static_cast<ThreatType_v3>(i)) << '\n';
	}

	FeatureExtractor_v3::FeatureExtractor_v3(GameConfig gameConfig) :
			game_config(gameConfig),
			pad(get_padding(gameConfig.rules)),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			raw_features(internal_board.rows(), internal_board.cols()),
			feature_types(gameConfig.rows, gameConfig.cols),
			threat_types(gameConfig.rows, gameConfig.cols),
			features_init("features init  "),
			features_class("features class "),
			threats_init("threats init   "),
			features_update("features update"),
			threats_update("threats update "),
//			feature_value_statistics(2 * (1 << (2 * 5))),
			feature_value_statistics(2 * (1 << (2 * 7))),
			feature_value_table(feature_value_statistics.size()),
			feature_table(&get_feature_table(gameConfig.rules)),
			threat_table(&get_threat_table(gameConfig.rules))
	{
		internal_board.fill(3);
	}

	void FeatureExtractor_v3::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;

		static_assert(sizeof(Sign) == sizeof(int16_t));
		for (int row = 0; row < board.rows(); row++)
			std::memcpy(internal_board.data(pad + row) + pad, board.data(row), sizeof(Sign) * board.cols());

		root_depth = Board::numberOfMoves(board);
		features_init.startTimer();
		calculate_raw_features();
		features_init.stopTimer();

		features_class.startTimer();
		classify_feature_types();
		features_class.stopTimer();

		threats_init.startTimer();
		get_threat_lists();
		threats_init.stopTimer();

//		if (features_init.getTotalCount() % 1000 == 0)
//		{
//			uint64_t max_count_x = 0, max_count_o = 0;
//			for (int i = 0; i < game_config.rows * game_config.cols; i++)
//			{
//				max_count_x = std::max(max_count_x, feature_value_statistics[2 * i]);
//				max_count_o = std::max(max_count_o, feature_value_statistics[2 * i + 1]);
//			}
//			for (int i = 0; i < game_config.rows * game_config.cols; i++)
//			{
//				feature_value_table[2 * i] = (255 * feature_value_statistics[2 * i]) / std::max(1ul, max_count_x);
//				feature_value_table[2 * i + 1] = (255 * feature_value_statistics[2 * i + 1]) / std::max(1ul, max_count_o);
//			}
//			for (size_t i = 0; i < feature_value_statistics.size(); i++)
//				feature_value_table[i] = (63 * feature_value_statistics[i]) / std::max(1ul, max_count);
//		}
	}

	void FeatureExtractor_v3::printRawFeature(int row, int col) const
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
				switch (getFeatureTypeAt(static_cast<Sign>(s), row, col, static_cast<Direction>(i)))
				{
					case FeatureType::NONE:
						std::cout << "NONE";
						break;
					case FeatureType::OPEN_3:
						std::cout << "OPEN_3";
						break;
					case FeatureType::HALF_OPEN_4:
						std::cout << "HALF_OPEN_4";
						break;
					case FeatureType::OPEN_4:
						std::cout << "OPEN_4";
						break;
					case FeatureType::DOUBLE_4:
						std::cout << "DOUBLE_4";
						break;
					case FeatureType::FIVE:
						std::cout << "FIVE";
						break;
					case FeatureType::OVERLINE:
						std::cout << "OVERLINE";
						break;
				}
			}
			std::cout << '\n';
		}
	}

	uint32_t FeatureExtractor_v3::getRawFeatureAt(int row, int col, Direction dir) const noexcept
	{
		return raw_features.at(pad + row, pad + col).get(dir);
	}
	FeatureType FeatureExtractor_v3::getFeatureTypeAt(Sign sign, int row, int col, Direction dir) const noexcept
	{
		assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
		if (sign == Sign::CROSS)
			return feature_types.at(row, col).for_cross[static_cast<int>(dir)];
		else
			return feature_types.at(row, col).for_circle[static_cast<int>(dir)];
	}
	ThreatType_v3 FeatureExtractor_v3::getThreatAt(Sign sign, int row, int col) const noexcept
	{
		switch (sign)
		{
			default:
				return ThreatType_v3::NONE;
			case Sign::CROSS:
				return threat_types.at(row, col).for_cross;
			case Sign::CIRCLE:
				return threat_types.at(row, col).for_circle;
		}
	}

	void FeatureExtractor_v3::printThreat(int row, int col) const
	{
		std::cout << "Threat for cross at  (" << row << ", " << col << ") = " << ag::toString(threat_types.at(row, col).for_cross) << '\n';
		std::cout << "Threat for circle at (" << row << ", " << col << ") = " << ag::toString(threat_types.at(row, col).for_circle) << '\n';
	}
	void FeatureExtractor_v3::printAllThreats() const
	{
		std::cout << "Threats-for-cross-----------\n";
		cross_threats.print();
		std::cout << "Threats-for-circle----------\n";
		circle_threats.print();
	}
	void FeatureExtractor_v3::print(Move lastMove) const
	{
		std::cout << "sign to move = " << sign_to_move << '\n';
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
//		for (int i = 0; i < internal_board.rows(); i++)
//		{
//			for (int j = 0; j < internal_board.cols(); j++)
//				switch (internal_board.at(i, j))
//				{
//					case 0:
//						std::cout << "_ ";
//						break;
//					case 1:
//						std::cout << "X ";
//						break;
//					case 2:
//						std::cout << "O ";
//						break;
//					case 3:
//						std::cout << "| ";
//						break;
//				}
//			std::cout << '\n';
//		}
		std::cout << std::endl;
	}

	void FeatureExtractor_v3::addMove(Move move) noexcept
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
	void FeatureExtractor_v3::undoMove(Move move) noexcept
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
//	void FeatureExtractor_v3::updateValueStatistics(Move move) noexcept
//	{
//		const int idx = move.row * game_config.cols + move.col;
//		feature_value_statistics[2 * idx + static_cast<int>(move.sign) - 1]++;
//		return;
//
//		const FeatureTypeGroup &ftg = feature_types.at(move.row, move.col);
//		const std::array<FeatureType, 4> group = (move.sign == Sign::CROSS) ? ftg.for_cross : ftg.for_circle;
//		for (int dir = 0; dir < 4; dir++)
//			if (group[dir] == FeatureType::NONE) // update only directions other than those that have threats
//			{
//				uint32_t raw_feature = getRawFeatureAt(move.row, move.col, static_cast<Direction>(dir));
////				raw_feature = (raw_feature >> (2 * pad - 4)) & 975;
//				raw_feature = (raw_feature >> (2 * pad - 6)) & 16191;
//				assert(raw_feature < feature_value_statistics.size());
//				feature_value_statistics[2 * raw_feature + static_cast<int>(move.sign) - 1]++;
//			}
//	}
//	uint8_t FeatureExtractor_v3::getValue(Move move) const noexcept
//	{
//		const int idx = move.row * game_config.cols + move.col;
//		return feature_value_table[2 * idx + static_cast<int>(move.sign) - 1];
//
//		uint8_t result = 0;
//		for (int dir = 0; dir < 4; dir++)
//		{
//			uint32_t raw_feature = getRawFeatureAt(move.row, move.col, static_cast<Direction>(dir));
////			raw_feature = (raw_feature >> (2 * pad - 4)) & 975;
//			raw_feature = (raw_feature >> (2 * pad - 6)) & 16191;
//			assert(raw_feature < feature_value_table.size());
//			result += feature_value_table[2 * raw_feature + static_cast<int>(move.sign) - 1];
//		}
//		return result;
//	}
	void FeatureExtractor_v3::print_stats() const
	{
		std::cout << features_init.toString() << '\n';
		std::cout << features_class.toString() << '\n';
		std::cout << threats_init.toString() << '\n';
		std::cout << features_update.toString() << '\n';
		std::cout << threats_update.toString() << '\n';

//		for (size_t i = 0; i < feature_value_table.size() / 2; i++)
//			if ((i & 48) == 0)
//			{
//				size_t feature = i;
//				for (int j = 0; j < 5; j++, feature >>= 2)
//					std::cout << text(static_cast<Sign>(feature & 3));
//				std::cout << " : " << (int) feature_value_table[2 * i] << " : " << (int) feature_value_table[2 * i + 1] << '\n';
//			}

//		size_t total_count = std::accumulate(feature_value_statistics.begin(), feature_value_statistics.end(), 0ull);
//		for (size_t i = 0; i < feature_value_statistics.size(); i++)
//			if ((i & 48) == 0)
//			{
//				size_t feature = i;
//				for (int j = 0; j < 5; j++, feature >>= 2)
//					std::cout << text(static_cast<Sign>(feature & 3));
//				std::cout << " : " << 100.0 * feature_value_statistics[i] / total_count << '\n';
//			}
	}
	/*
	 * private
	 */
	void FeatureExtractor_v3::calculate_raw_features() noexcept
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
	void FeatureExtractor_v3::classify_feature_types() noexcept
	{
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (int dir = 0; dir < 4; dir++)
					{
						const FeatureEncoding tmp = feature_table->getFeatureType(getRawFeatureAt(row, col, static_cast<Direction>(dir)));
						feature_types.at(row, col).for_cross[dir] = tmp.forCross();
						feature_types.at(row, col).for_circle[dir] = tmp.forCircle();
					}
				}
				else
					feature_types.at(row, col) = FeatureTypeGroup();
	}
	void FeatureExtractor_v3::get_threat_lists()
	{
		cross_threats.clear();
		circle_threats.clear();

		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					const Threat_v3 threat = threat_table->getThreat(feature_types.at(row, col));
					threat_types.at(row, col) = threat;

					cross_threats.add(threat.for_cross, Move(row, col));
					circle_threats.add(threat.for_circle, Move(row, col));
				}
				else
					threat_types.at(row, col) = Threat_v3();
	}

	void FeatureExtractor_v3::update_central_spot(int row, int col, int mode) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);
		assert(mode == 1 || mode == -1);
		assert(signAt(row, col) == Sign::NONE);

		for (int dir = 0; dir < 4; dir++)
			central_spot_encoding[dir] = feature_table->getFeatureType(getRawFeatureAt(row, col, static_cast<Direction>(dir)));

		if (mode == 1) // a stone was added
		{
			const Threat_v3 old_threat = threat_types.at(row, col);
			cross_threats.remove(old_threat.for_cross, Move(row, col));
			circle_threats.remove(old_threat.for_circle, Move(row, col));

			// since this spot is now occupied, there are no threats
			feature_types.at(row, col) = FeatureTypeGroup();
			threat_types.at(row, col) = Threat_v3();
		}
		else // a stone was removed
		{
			for (int dir = 0; dir < 4; dir++)
			{
				feature_types.at(row, col).for_cross[dir] = central_spot_encoding[dir].forCross();
				feature_types.at(row, col).for_circle[dir] = central_spot_encoding[dir].forCircle();
			}
			const Threat_v3 new_threat = threat_table->getThreat(feature_types.at(row, col));

			threat_types.at(row, col) = new_threat; // since the spot is now empty, new threats might have been created
			cross_threats.add(new_threat.for_cross, Move(row, col));
			circle_threats.add(new_threat.for_circle, Move(row, col));
		}
	}
	void FeatureExtractor_v3::update_neighborhood(int row, int col) noexcept
	{
		for (int i = -pad; i <= pad; i++)
			if (i != 0) // central spot must have been updated anyway, so it is not included in the lookup table
			{
				const int idx = pad + i - static_cast<int>(i > 0);
				if (central_spot_encoding[HORIZONTAL].mustBeUpdated(idx))
					update_feature_types_and_threats(row, col + i, HORIZONTAL);
				if (central_spot_encoding[VERTICAL].mustBeUpdated(idx))
					update_feature_types_and_threats(row + i, col, VERTICAL);
				if (central_spot_encoding[DIAGONAL].mustBeUpdated(idx))
					update_feature_types_and_threats(row + i, col + i, DIAGONAL);
				if (central_spot_encoding[ANTIDIAGONAL].mustBeUpdated(idx))
					update_feature_types_and_threats(row + i, col - i, ANTIDIAGONAL);
			}
	}
	void FeatureExtractor_v3::update_feature_types_and_threats(int row, int col, int direction) noexcept
	{
		assert(row >= 0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		// find new feature type and update appropriate direction in the table
		const FeatureEncoding new_feature_type = feature_table->getFeatureType(getRawFeatureAt(row, col, static_cast<Direction>(direction)));
		feature_types.at(row, col).for_cross[direction] = new_feature_type.forCross();
		feature_types.at(row, col).for_circle[direction] = new_feature_type.forCircle();

		const Threat_v3 old_threat = threat_types.at(row, col); // check what was the best threat at (row, col) before updating
		const Threat_v3 new_threat = threat_table->getThreat(feature_types.at(row, col)); // find new threat type according to the newly updated feature types
		threat_types.at(row, col) = new_threat;

		// update list of threats for cross and circle (if necessary)
		if (old_threat.for_cross != new_threat.for_cross)
		{
			cross_threats.remove(old_threat.for_cross, Move(row, col));
			cross_threats.add(new_threat.for_cross, Move(row, col));
		}
		if (old_threat.for_circle != new_threat.for_circle)
		{
			circle_threats.remove(old_threat.for_circle, Move(row, col));
			circle_threats.add(new_threat.for_circle, Move(row, col));
		}
	}

} /* namespace ag */

