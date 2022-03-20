/*
 * FeatureExtractor.cpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureExtractor_v2.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <algorithm>
#include <cstring>

namespace
{
	using namespace ag;

	template<int Pad>
	void horizontal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
		for (int row = Pad; row < board.rows() - Pad; row++)
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int col = Pad; col <= 2 * Pad; col++)
				line = line | (board.at(row, col) << (col * 2));
			features.at(row, Pad).horizontal = line;
			for (int col = Pad + 1; col < board.cols() - Pad; col++)
			{
				line = (board.at(row, col + Pad) << shift) | (line >> 2);
				features.at(row, col).horizontal = line;
			}
		}
	}
	template<int Pad>
	void vertical(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
		for (int col = Pad; col < board.cols() - Pad; col++)
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int row = Pad; row <= 2 * Pad; row++)
				line = line | (board.at(row, col) << (row * 2));
			features.at(Pad, col).vertical = line;
			for (int row = Pad + 1; row < board.rows() - Pad; row++)
			{
				line = (board.at(row + Pad, col) << shift) | (line >> 2);
				features.at(row, col).vertical = line;
			}
		}
	}
	template<int Pad>
	void diagonal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
		for (int i = 0; i < board.rows() - 2 * Pad; i++) // lower left half
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int j = Pad; j <= 2 * Pad; j++)
				line = line | (board.at(i + j, j) << (j * 2));
			features.at(Pad + i, Pad).diagonal = line;
			for (int j = Pad + 1; j < board.cols() - Pad - i; j++)
			{
				line = (board.at(Pad + i + j, Pad + j) << shift) | (line >> 2);
				features.at(i + j, j).diagonal = line;
			}
		}

		for (int i = 1; i < board.cols() - 2 * Pad; i++) // upper right half
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int j = Pad; j <= 2 * Pad; j++)
				line = line | (board.at(j, i + j) << (j * 2));
			features.at(Pad, Pad + i).diagonal = line;
			for (int j = Pad + 1; j < board.rows() - Pad - i; j++)
			{
				line = (board.at(Pad + j, Pad + i + j) << shift) | (line >> 2);
				features.at(j, j + i).diagonal = line;
			}
		}
	}
	template<int Pad>
	void antidiagonal(matrix<FeatureGroup> &features, const matrix<int16_t> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * Pad;
		for (int i = 0; i < board.rows() - 2 * Pad; i++) // upper left half
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int j = 0; j <= Pad; j++)
				line = line | (board.at(Pad + i - j, Pad + j) << ((Pad + j) * 2));
			features.at(Pad + i, Pad).antidiagonal = line;
			for (int j = 1; j <= i; j++)
			{
				line = (board.at(Pad + i - j - Pad, Pad + j + Pad) << shift) | (line >> 2);
				features.at(Pad + i - j, Pad + j).antidiagonal = line;
			}
		}

		for (int i = 0; i < board.cols() - 2 * Pad; i++) // lower right half
		{
			uint32_t line = (1 << (2 * Pad)) - 1;
			for (int j = 0; j <= Pad; j++)
				line = line | (board.at(board.rows() - Pad - 1 - j, Pad + i + j) << ((Pad + j) * 2));
			features.at(board.rows() - Pad - 1, Pad + i).antidiagonal = line;
			for (int j = 1; j < board.cols() - 2 * Pad - i; j++)
			{
				line = (board.at(board.rows() - Pad - 1 - j - Pad, Pad + i + j + Pad) << shift) | (line >> 2);
				features.at(board.rows() - Pad - 1 - j, Pad + i + j).antidiagonal = line;
			}
		}
	}

	template<int Pad>
	void add_move(matrix<FeatureGroup> &features, matrix<int16_t> &board, Move move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != Sign::NONE);
		assert(board.at(Pad + move.row, Pad + move.col) == 0); // move must be made on empty spot

		board.at(Pad + move.row, Pad + move.col) = static_cast<int>(move.sign);

		// first calculate rows above move
		uint32_t mask1 = static_cast<uint32_t>(move.sign) << (4 * Pad);
		uint32_t mask2 = static_cast<uint32_t>(move.sign);
		for (int i = 0; i <= Pad; i++)
		{
			features.at(move.row + i, Pad + move.col - (Pad - i)).diagonal |= mask1;
			features.at(move.row + i, Pad + move.col).vertical |= mask1;
			features.at(move.row + i, Pad + move.col + (Pad - i)).antidiagonal |= mask2;
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}

		uint32_t mask3 = static_cast<uint32_t>(move.sign) << (4 * Pad);
		for (int i = 0; i <= 2 * Pad; i++)
		{
			features.at(Pad + move.row, move.col + i).horizontal |= mask3;
			mask3 = mask3 >> 2;
		}

		for (int i = 1; i <= Pad; i++)
		{
			features.at(Pad + move.row + i, Pad + move.col - i).antidiagonal |= mask2;
			features.at(Pad + move.row + i, Pad + move.col).vertical |= mask1;
			features.at(Pad + move.row + i, Pad + move.col + i).diagonal |= mask1;
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}
	}
	template<int Pad>
	void undo_move(matrix<FeatureGroup> &features, matrix<int16_t> &board, Move move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != Sign::NONE);
		assert(board.at(Pad + move.row, Pad + move.col) == static_cast<int>(move.sign));

		board.at(Pad + move.row, Pad + move.col) = static_cast<int>(Sign::NONE);

		// first calculate rows above move
		uint32_t mask1 = ~(3 << (4 * Pad));
		uint32_t mask2 = ~3;
		for (int i = 0; i <= Pad; i++)
		{
			features.at(move.row + i, Pad + move.col - (Pad - i)).diagonal &= mask1;
			features.at(move.row + i, Pad + move.col).vertical &= mask1;
			features.at(move.row + i, Pad + move.col + (Pad - i)).antidiagonal &= mask2;
			mask1 = (3 << 30) | (mask1 >> 2);
			mask2 = (mask2 << 2) | 3;
		}

		uint32_t mask3 = ~(3 << (4 * Pad));
		for (int i = 0; i <= 2 * Pad; i++)
		{
			features.at(Pad + move.row, move.col + i).horizontal &= mask3;
			mask3 = (3 << 30) | (mask3 >> 2);
		}

		for (int i = 1; i <= Pad; i++)
		{
			features.at(Pad + move.row + i, Pad + move.col - i).antidiagonal &= mask2;
			features.at(Pad + move.row + i, Pad + move.col).vertical &= mask1;
			features.at(Pad + move.row + i, Pad + move.col + i).diagonal &= mask1;
			mask1 = (3 << 30) | (mask1 >> 2);
			mask2 = (mask2 << 2) | 3;
		}
	}

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
	const FeatureTable& get_feature_table(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
			{
				static FeatureTable table(GameRules::FREESTYLE);
				return table;
			}
			case GameRules::STANDARD:
			{
				static FeatureTable table(GameRules::STANDARD);
				return table;
			}
			case GameRules::RENJU:
			{
				static FeatureTable table(GameRules::RENJU);
				return table;
			}
			case GameRules::CARO:
			{
				static FeatureTable table(GameRules::CARO);
				return table;
			}
			default:
				throw std::logic_error("unknown rule");
		}
	}

//	Threat find_4x4_fork(const ThreatGroup &t) noexcept
//	{
//		int cross = 0, circle = 0;
//		auto check_direction = [&](Threat t)
//		{	cross += (t.for_cross == ThreatType::HALF_OPEN_FOUR);
//			circle += (t.for_circle == ThreatType::HALF_OPEN_FOUR);
//		};
//		check_direction(t.horizontal);
//		check_direction(t.vertical);
//		check_direction(t.diagonal);
//		check_direction(t.antidiagonal);
//
//		Threat result;
//		result.for_cross = (cross >= 2) ? ThreatType::OPEN_FOUR : ThreatType::NONE;
//		result.for_circle = (circle >= 2) ? ThreatType::OPEN_FOUR : ThreatType::NONE;
//		return result;
//	}
}

namespace ag
{

	FeatureExtractor_v2::FeatureExtractor_v2(GameConfig gameConfig) :
			game_config(gameConfig),
			pad(get_padding(gameConfig.rules)),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			features(internal_board.rows(), internal_board.cols()),
			threats(gameConfig.rows, gameConfig.cols)
	{
		internal_board.fill(3);
		cross_five.reserve(64);
		cross_open_four.reserve(64);
		cross_half_open_four.reserve(64);

		circle_five.reserve(64);
		circle_open_four.reserve(64);
		circle_half_open_four.reserve(64);
	}

	void FeatureExtractor_v2::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;

		static_assert(sizeof(Sign) == sizeof(int16_t));
		for (int row = 0; row < board.rows(); row++)
			std::memcpy(internal_board.data(pad + row) + pad, board.data(row), sizeof(Sign) * board.cols());

		root_depth = Board::numberOfMoves(board);
		calc_all_features();
		get_threat_lists();
	}

	void FeatureExtractor_v2::printFeature(int row, int col) const
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

			uint32_t line = get_feature_at(row, col, static_cast<Direction>(i));
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
//			std::cout << " : (" << features.at(pad + row, (pad + col) * 4 + i) << ")\n";
		}
	}

	uint32_t FeatureExtractor_v2::getFeatureAt(int row, int col, Direction dir) const noexcept
	{
		return features.at(pad + row, pad + col).get(dir);
	}
	ThreatType FeatureExtractor_v2::getThreatAt(Sign sign, int row, int col, Direction dir) const noexcept
	{
		switch (sign)
		{
			default:
				return ThreatType::NONE;
			case Sign::CROSS:
				return threats.at(row, col).get(dir).for_cross;
			case Sign::CIRCLE:
				return threats.at(row, col).get(dir).for_circle;
		}
	}

	void FeatureExtractor_v2::printThreat(int row, int col) const
	{
		const FeatureTable &table = get_feature_table(game_config.rules);
		std::cout << "Threat for cross at  (" << row << ", " << col << ") = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(table.getThreat(get_feature_at(row, col, static_cast<Direction>(i))).for_cross) << ", ";
		std::cout << "in map = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(getThreatAt(Sign::CROSS, row, col, static_cast<Direction>(i))) << ", ";
		std::cout << '\n';

		std::cout << "Threat for circle at (" << row << ", " << col << ") = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(table.getThreat(get_feature_at(row, col, static_cast<Direction>(i))).for_circle) << ", ";
		std::cout << "in map = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(getThreatAt(Sign::CIRCLE, row, col, static_cast<Direction>(i))) << ", ";
		std::cout << '\n';
	}
	void FeatureExtractor_v2::printAllThreats() const
	{
		std::cout << "Threats-for-cross-----------\n";
		for (size_t i = 0; i < cross_five.size(); i++)
			std::cout << cross_five[i].toString() << " : " << ag::toString(ThreatType::FIVE) << '\n';
		for (size_t i = 0; i < cross_open_four.size(); i++)
			std::cout << cross_open_four[i].toString() << " : " << ag::toString(ThreatType::OPEN_FOUR) << '\n';
		for (size_t i = 0; i < cross_half_open_four.size(); i++)
			std::cout << cross_half_open_four[i].toString() << " : " << ag::toString(ThreatType::HALF_OPEN_FOUR) << '\n';
		std::cout << "Threats-for-circle----------\n";
		for (size_t i = 0; i < circle_five.size(); i++)
			std::cout << circle_five[i].toString() << " : " << ag::toString(ThreatType::FIVE) << '\n';
		for (size_t i = 0; i < circle_open_four.size(); i++)
			std::cout << circle_open_four[i].toString() << " : " << ag::toString(ThreatType::OPEN_FOUR) << '\n';
		for (size_t i = 0; i < circle_half_open_four.size(); i++)
			std::cout << circle_half_open_four[i].toString() << " : " << ag::toString(ThreatType::HALF_OPEN_FOUR) << '\n';
	}
	void FeatureExtractor_v2::print() const
	{
		std::cout << "sign to move = " << sign_to_move << '\n';
		for (int i = 0; i < internal_board.rows(); i++)
		{
			for (int j = 0; j < internal_board.cols(); j++)
				switch (internal_board.at(i, j))
				{
					case 0:
						std::cout << "_ ";
						break;
					case 1:
						std::cout << "X ";
						break;
					case 2:
						std::cout << "O ";
						break;
					case 3:
						std::cout << "| ";
						break;
				}
			std::cout << '\n';
		}
		std::cout << '\n';
	}

	void FeatureExtractor_v2::addMove(Move move) noexcept
	{
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				add_move<4>(features, internal_board, move);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				add_move<5>(features, internal_board, move);
				break;
			default:
				break;
		}

		update_threats(move.row, move.col);
	}
	void FeatureExtractor_v2::undoMove(Move move) noexcept
	{
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				undo_move<4>(features, internal_board, move);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				undo_move<5>(features, internal_board, move);
				break;
			default:
				break;
		}

		update_threats(move.row, move.col);
	}
	/*
	 * private
	 */
	uint32_t FeatureExtractor_v2::get_feature_at(int row, int col, Direction dir) const noexcept
	{
		return features.at(pad + row, pad + col).get(dir);
	}
	void FeatureExtractor_v2::calc_all_features() noexcept
	{
		switch (game_config.rules)
		{
			case GameRules::FREESTYLE:
				horizontal<4>(features, internal_board);
				vertical<4>(features, internal_board);
				diagonal<4>(features, internal_board);
				antidiagonal<4>(features, internal_board);
				break;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				horizontal<5>(features, internal_board);
				vertical<5>(features, internal_board);
				diagonal<5>(features, internal_board);
				antidiagonal<5>(features, internal_board);
				break;
			default:
				break;
		}
	}
	void FeatureExtractor_v2::get_threat_lists()
	{
		cross_five.clear();
		cross_open_four.clear();
		cross_half_open_four.clear();
		circle_five.clear();
		circle_open_four.clear();
		circle_half_open_four.clear();

		const FeatureTable &table = get_feature_table(game_config.rules);
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++)
				if (signAt(row, col) == Sign::NONE)
				{
					for (int i = 0; i < 4; i++)
					{
						const Direction dir = static_cast<Direction>(i);
						threats.at(row, col).get(dir) = table.getThreat(get_feature_at(row, col, dir));
					}

					const Threat best_threat = threats.at(row, col).best();
					add_threat<Sign::CROSS>(best_threat.for_cross, Move(row, col));
					add_threat<Sign::CIRCLE>(best_threat.for_circle, Move(row, col));
				}
				else
					threats.at(row, col) = ThreatGroup();
	}
	void FeatureExtractor_v2::update_threats(int row, int col)
	{
		const FeatureTable &table = get_feature_table(game_config.rules);

#define UPDATE_THREAT_AT(r, c, dir)\
	if ((c) >= 0 && (c) < game_config.cols && signAt((r), (c)) == Sign::NONE)\
		update_threat_at(table, (r), (c), dir);

		for (int i = std::min(row, pad); i > 0; i--) // update rows above and the center spot
		{
			UPDATE_THREAT_AT(row - i, col - i, Direction::DIAGONAL)
			UPDATE_THREAT_AT(row - i, col, Direction::VERTICAL)
			UPDATE_THREAT_AT(row - i, col + i, Direction::ANTIDIAGONAL)
		}
		for (int i = col - pad; i < col; i++) // update the same row as placed move
		{
			UPDATE_THREAT_AT(row, i, Direction::HORIZONTAL)
		}

		update_central_threat(table, row, col);

		for (int i = col + 1; i <= col + pad; i++) // update the same row as placed move
		{
			UPDATE_THREAT_AT(row, i, Direction::HORIZONTAL)
		}
		for (int i = 1; i <= std::min(game_config.rows - row, pad); i++) // update rows below
		{
			UPDATE_THREAT_AT(row + i, col - i, Direction::ANTIDIAGONAL)
			UPDATE_THREAT_AT(row + i, col, Direction::VERTICAL)
			UPDATE_THREAT_AT(row + i, col + i, Direction::DIAGONAL)
		}
#undef UPDATE_THREAT_AT
	}
	void FeatureExtractor_v2::update_threat_at(const FeatureTable &table, int row, int col, Direction direction)
	{
		assert(row >=0 && row < game_config.rows && col >= 0 && col < game_config.cols);
		assert(signAt(row, col) == Sign::NONE);
		// check what was the best threat at (row, col) before updating
		const Threat old_best_threat = threats.at(row, col).best();

		// update the threat tables with newly formed threat in the given direction
		const Threat new_threat = table.getThreat(get_feature_at(row, col, direction)); // this is new threat as taken from feature table
		threats.at(row, col).get(direction) = new_threat;

		// check what is the new best threat at (row, col) after threat tables was updated
		const Threat new_best_threat = threats.at(row, col).best();

		// update list of threats for cross and circle
		if (old_best_threat.for_cross != new_best_threat.for_cross)
		{
			remove_threat<Sign::CROSS>(old_best_threat.for_cross, Move(row, col));
			add_threat<Sign::CROSS>(new_best_threat.for_cross, Move(row, col));
		}
		if (old_best_threat.for_circle != new_best_threat.for_circle)
		{
			remove_threat<Sign::CIRCLE>(old_best_threat.for_circle, Move(row, col));
			add_threat<Sign::CIRCLE>(new_best_threat.for_circle, Move(row, col));
		}
	}
	void FeatureExtractor_v2::update_central_threat(const FeatureTable &table, int row, int col)
	{
		assert(row >=0 && row < game_config.rows && col >= 0 && col < game_config.cols);

		if (signAt(row, col) != Sign::NONE) // stone was placed at this spot
		{
			const Threat old_best_threat = threats.at(row, col).best();
			threats.at(row, col) = ThreatGroup(); // since this spot is occupied, there are no threats
			remove_threat<Sign::CROSS>(old_best_threat.for_cross, Move(row, col));
			remove_threat<Sign::CIRCLE>(old_best_threat.for_circle, Move(row, col));
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				const Direction dir = static_cast<Direction>(i);
				threats.at(row, col).get(dir) = table.getThreat(get_feature_at(row, col, dir));
			}
			const Threat new_best_threat = threats.at(row, col).best();
			add_threat<Sign::CROSS>(new_best_threat.for_cross, Move(row, col));
			add_threat<Sign::CIRCLE>(new_best_threat.for_circle, Move(row, col));
		}
	}

} /* namespace ag */

