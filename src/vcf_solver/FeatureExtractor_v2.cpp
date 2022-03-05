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

namespace
{
	using namespace ag;

	template<int Pad>
	void horizontal(matrix<FeatureGroup> &features, const matrix<int> &board) noexcept
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
	void vertical(matrix<FeatureGroup> &features, const matrix<int> &board) noexcept
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
	void diagonal(matrix<FeatureGroup> &features, const matrix<int> &board) noexcept
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
	void antidiagonal(matrix<FeatureGroup> &features, const matrix<int> &board) noexcept
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
	void add_move(matrix<FeatureGroup> &features, matrix<int> &board, const Move &move) noexcept
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
			features.at(move.row + i, Pad + move.col - (Pad - i)).diagonal |= mask1; // diagonal
			features.at(move.row + i, Pad + move.col).vertical |= mask1; // vertical
			features.at(move.row + i, Pad + move.col + (Pad - i)).antidiagonal |= mask2; // antidiagonal
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}

		uint32_t mask3 = static_cast<uint32_t>(move.sign) << (4 * Pad);
		for (int i = 0; i <= 2 * Pad; i++)
		{
			features.at(Pad + move.row, move.col + i).horizontal |= mask3; // horizontal
			mask3 = mask3 >> 2;
		}

		for (int i = 1; i <= Pad; i++)
		{
			features.at(Pad + move.row + i, Pad + move.col - i).antidiagonal |= mask2; // antidiagonal
			features.at(Pad + move.row + i, Pad + move.col).vertical |= mask1; // vertical
			features.at(Pad + move.row + i, Pad + move.col + i).diagonal |= mask1; // diagonal
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}
	}
	template<int Pad>
	void undo_move(matrix<FeatureGroup> &features, matrix<int> &board, const Move &move) noexcept
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
			features.at(move.row + i, Pad + move.col - (Pad - i)).diagonal &= mask1; // diagonal
			features.at(move.row + i, Pad + move.col).vertical &= mask1; // vertical
			features.at(move.row + i, Pad + move.col + (Pad - i)).antidiagonal &= mask2; // antidiagonal
			mask1 = (3 << 30) | (mask1 >> 2);
			mask2 = (mask2 << 2) | 3;
		}

		uint32_t mask3 = ~(3 << (4 * Pad));
		for (int i = 0; i <= 2 * Pad; i++)
		{
			features.at(Pad + move.row, move.col + i).horizontal &= mask3; // horizontal
			mask3 = (3 << 30) | (mask3 >> 2);
		}

		for (int i = 1; i <= Pad; i++)
		{
			features.at(Pad + move.row + i, Pad + move.col - i).antidiagonal &= mask2; // antidiagonal
			features.at(Pad + move.row + i, Pad + move.col).vertical &= mask1; // vertical
			features.at(Pad + move.row + i, Pad + move.col + i).diagonal &= mask1; // diagonal
			mask1 = (3 << 30) | (mask1 >> 2);
			mask2 = (mask2 << 2) | 3;
		}
	}

	int get_padding(GameRules rules)
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
				throw std::logic_error("unknown rule");
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
	}

	Sign FeatureExtractor_v2::signAt(int row, int col) const noexcept
	{
		return static_cast<Sign>(internal_board.at(pad + row, pad + col));
	}
	void FeatureExtractor_v2::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
				internal_board.at(pad + row, pad + col) = static_cast<int>(board.at(row, col));
		root_depth = Board::numberOfMoves(board);
		calc_all_features();
		get_threat_lists();
	}
	Sign FeatureExtractor_v2::getSignToMove() const noexcept
	{
		return sign_to_move;
	}
	int FeatureExtractor_v2::getRootDepth() const noexcept
	{
		return root_depth;
	}
	const std::vector<Move>& FeatureExtractor_v2::getThreats(ThreatType type, Sign sign) const noexcept
	{
		assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
		assert(type != ThreatType::NONE);
		switch (type)
		{
			default:
			case ThreatType::HALF_OPEN_FOUR:
				return (sign == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
			case ThreatType::OPEN_FOUR:
				return (sign == Sign::CROSS) ? cross_open_four : circle_open_four;
			case ThreatType::FIVE:
				return (sign == Sign::CROSS) ? cross_five : circle_five;
		}
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
			std::cout << ag::toString(table.getThreat_v2(get_feature_at(row, col, static_cast<Direction>(i))).for_cross) << ", ";
		std::cout << "in map = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(getThreatAt(Sign::CROSS, row, col, static_cast<Direction>(i))) << ", ";
		std::cout << '\n';

		std::cout << "Threat for circle at (" << row << ", " << col << ") = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(table.getThreat_v2(get_feature_at(row, col, static_cast<Direction>(i))).for_circle) << ", ";
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
		features.clear();
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
				if (internal_board.at(pad + row, pad + col) == 0)
				{
					for (int dir = 0; dir < 4; dir++)
					{
						const Direction direction = static_cast<Direction>(dir);
						threats.at(row, col).get(direction) = table.getThreat_v2(get_feature_at(row, col, direction));
					}

					Threat best_threat = threats.at(row, col).best();

					switch (best_threat.for_cross)
					{
						case ThreatType::NONE:
							break;
						case ThreatType::HALF_OPEN_FOUR:
							cross_half_open_four.push_back(Move(row, col));
							break;
						case ThreatType::OPEN_FOUR:
							cross_open_four.push_back(Move(row, col));
							break;
						case ThreatType::FIVE:
							cross_five.push_back(Move(row, col));
							break;
					}

					switch (best_threat.for_circle)
					{
						case ThreatType::NONE:
							break;
						case ThreatType::HALF_OPEN_FOUR:
							circle_half_open_four.push_back(Move(row, col));
							break;
						case ThreatType::OPEN_FOUR:
							circle_open_four.push_back(Move(row, col));
							break;
						case ThreatType::FIVE:
							circle_five.push_back(Move(row, col));
							break;
					}
				}
				else
				{
					for (int dir = 0; dir < 4; dir++)
						threats.at(row, col).get(static_cast<Direction>(dir)) = Threat();
				}
	}
	void FeatureExtractor_v2::update_threats(int row, int col)
	{
		const FeatureTable &table = get_feature_table(game_config.rules);
		for (int i = 0; i <= pad; i++) // update rows above
		{
			update_threat_at(table, row - pad + i, col - pad + i, Direction::DIAGONAL);
			update_threat_at(table, row - pad + i, col, Direction::VERTICAL);
			update_threat_at(table, row - pad + i, col + pad - i, Direction::ANTIDIAGONAL);
		}
		for (int i = col - pad; i <= col + pad; i++) // update the same row as placed move
			update_threat_at(table, row, i, Direction::HORIZONTAL);
		for (int i = 1; i <= pad; i++) // update rows below
		{
			update_threat_at(table, row + i, col - i, Direction::ANTIDIAGONAL);
			update_threat_at(table, row + i, col, Direction::VERTICAL);
			update_threat_at(table, row + i, col + i, Direction::DIAGONAL);
		}
	}
	void FeatureExtractor_v2::update_threat_at(const FeatureTable &table, int row, int col, Direction direction)
	{
		if (row >= 0 and row < game_config.rows and col >= 0 and col < game_config.cols) // only inside board
		{
			// check what was the best threat at (row, col) before updating
			const Threat old_best_threat = threats.at(row, col).best();

			// update the threat tables with newly formed threat in the given direction
			const Threat new_threat = table.getThreat_v2(get_feature_at(row, col, direction)); // this is new threat as taken from feature table
			threats.at(row, col).get(direction) = new_threat;

			// check what is the new best threat at (row, col) after threat tables was updated
			const Threat new_best_threat = threats.at(row, col).best();

			// update list of threats for cross and circle
			update_threat_list(old_best_threat.for_cross, new_best_threat.for_cross, Move(row, col), cross_five, cross_open_four,
					cross_half_open_four);
			update_threat_list(old_best_threat.for_circle, new_best_threat.for_circle, Move(row, col), circle_five, circle_open_four,
					circle_half_open_four);
		}
	}
	void FeatureExtractor_v2::update_threat_list(ThreatType old_threat, ThreatType new_threat, Move move, std::vector<Move> &five,
			std::vector<Move> &open_four, std::vector<Move> &half_open_four)
	{
		if (old_threat == new_threat)
			return; // do not touch lists if the threat did not change

		// remove old threat from appropriate list
		switch (old_threat)
		{
			case ThreatType::NONE:
				break;
			case ThreatType::HALF_OPEN_FOUR:
			{
				auto index = std::find(half_open_four.begin(), half_open_four.end(), move);
				assert(index != half_open_four.end()); // the threat must exist in the list
				// 				as half-open fours are frequent, instead of deleting element from the middle, move it to the end first
				//				such shuffling of threats should improve search efficiency (on average)
				std::swap(*index, half_open_four.back());
				half_open_four.pop_back();
				break;
			}
			case ThreatType::OPEN_FOUR:
			{
				auto index = std::find(open_four.begin(), open_four.end(), move);
				assert(index != open_four.end()); // the threat must exist in the list
				std::swap(*index, open_four.back());
				open_four.pop_back();
//				open_four.erase(index);
				break;
			}
			case ThreatType::FIVE:
			{
				auto index = std::find(five.begin(), five.end(), move);
				assert(index != five.end());
				std::swap(*index, five.back());
				five.pop_back();
//				five.erase(index);
				break;
			}
		}

		// add new threat to appropriate list
		switch (new_threat)
		{
			case ThreatType::NONE:
				break;
			case ThreatType::HALF_OPEN_FOUR:
				half_open_four.push_back(move);
				break;
			case ThreatType::OPEN_FOUR:
				open_four.push_back(move);
				break;
			case ThreatType::FIVE:
				five.push_back(move);
				break;
		}
	}

} /* namespace ag */

