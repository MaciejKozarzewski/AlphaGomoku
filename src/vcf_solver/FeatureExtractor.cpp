/*
 * FeatureExtractor.cpp
 *
 *  Created on: 2 maj 2021
 *      Author: maciek
 */

#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>
#include <algorithm>

namespace
{
	using namespace ag;

	template<int pad>
	void horizontal(matrix<uint32_t> &features, const matrix<int> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * pad;
		for (int row = pad; row < board.rows() - pad; row++)
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int col = pad; col <= 2 * pad; col++)
				line = line | (board.at(row, col) << (col * 2));
			features.at(row, pad * 4) = line;
			for (int col = pad + 1; col < board.cols() - pad; col++)
			{
				line = (board.at(row, col + pad) << shift) | (line >> 2);
				features.at(row, col * 4) = line;
			}
		}
	}
	template<int pad>
	void vertical(matrix<uint32_t> &features, const matrix<int> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * pad;
		for (int col = pad; col < board.cols() - pad; col++)
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int row = pad; row <= 2 * pad; row++)
				line = line | (board.at(row, col) << (row * 2));
			features.at(pad, col * 4 + 1) = line;
			for (int row = pad + 1; row < board.rows() - pad; row++)
			{
				line = (board.at(row + pad, col) << shift) | (line >> 2);
				features.at(row, col * 4 + 1) = line;
			}
		}
	}
	template<int pad>
	void diagonal(matrix<uint32_t> &features, const matrix<int> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * pad;
		for (int i = 0; i < board.rows() - 2 * pad; i++) // lower left half
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int j = pad; j <= 2 * pad; j++)
				line = line | (board.at(i + j, j) << (j * 2));
			features.at(pad + i, pad * 4 + 2) = line;
			for (int j = pad + 1; j < board.cols() - pad - i; j++)
			{
				line = (board.at(pad + i + j, pad + j) << shift) | (line >> 2);
				features.at(i + j, j * 4 + 2) = line;
			}
		}

		for (int i = 1; i < board.cols() - 2 * pad; i++) // upper right half
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int j = pad; j <= 2 * pad; j++)
				line = line | (board.at(j, i + j) << (j * 2));
			features.at(pad, (pad + i) * 4 + 2) = line;
			for (int j = pad + 1; j < board.rows() - pad - i; j++)
			{
				line = (board.at(pad + j, pad + i + j) << shift) | (line >> 2);
				features.at(j, (j + i) * 4 + 2) = line;
			}
		}
	}
	template<int pad>
	void antidiagonal(matrix<uint32_t> &features, const matrix<int> &board) noexcept
	{
		assert(board.isSquare());
		const uint32_t shift = 4 * pad;
		for (int i = 0; i < board.rows() - 2 * pad; i++) // upper left half
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int j = 0; j <= pad; j++)
				line = line | (board.at(i + pad - j, pad + j) << ((pad + j) * 2));
			features.at(pad + i, pad * 4 + 3) = line;
			for (int j = 1; j < i; j++)
			{
				line = (board.at(pad + i - (pad + j), pad + (pad + j)) << shift) | (line >> 2);
				features.at(pad + i - j, (pad + j) * 4 + 3) = line;
			}
		}

		for (int i = 0; i < board.cols() - 2 * pad; i++) // lower right half
		{
			uint32_t line = (1 << (2 * pad)) - 1;
			for (int j = 0; j <= pad; j++)
				line = line | (board.at(board.rows() - pad - 1 - j, pad + i + j) << ((pad + j) * 2));
			features.at(board.rows() - pad - 1, (pad + i) * 4 + 3) = line;
			for (int j = 1; j < board.cols() - 2 * pad - i; j++)
			{
				line = (board.at(board.rows() - pad - 1 - j - pad, pad + i + j + pad) << shift) | (line >> 2);
				features.at(board.rows() - pad - 1 - j, (pad + i + j) * 4 + 3) = line;
			}
		}
	}

	template<int pad>
	void add_move(matrix<uint32_t> &features, matrix<int> &board, const Move &move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != Sign::NONE);
		assert(board.at(pad + move.row, pad + move.col) == 0); // move must be made on empty spot

		board.at(pad + move.row, pad + move.col) = static_cast<int>(move.sign);

		// first calculate rows above move
		uint32_t mask1 = static_cast<uint32_t>(move.sign) << (4 * pad);
		uint32_t mask2 = static_cast<uint32_t>(move.sign);
		for (int i = 0; i <= pad; i++)
		{
			features.at(move.row + i, (pad + move.col - (pad - i)) * 4 + 2) |= mask1; // diagonal
			features.at(move.row + i, (pad + move.col) * 4 + 1) |= mask1; // vertical
			features.at(move.row + i, (pad + move.col + (pad - i)) * 4 + 3) |= mask2; // antidiagonal
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}

		uint32_t mask3 = static_cast<uint32_t>(move.sign) << (4 * pad);
		for (int i = 0; i <= 2 * pad; i++)
		{
			features.at(pad + move.row, (move.col + i) * 4) |= mask3; // horizontal
			mask3 = mask3 >> 2;
		}

		for (int i = 1; i <= pad; i++)
		{
			features.at(pad + move.row + i, (pad + move.col - i) * 4 + 3) |= mask2; // antidiagonal
			features.at(pad + move.row + i, (pad + move.col) * 4 + 1) |= mask1; // vertical
			features.at(pad + move.row + i, (pad + move.col + i) * 4 + 2) |= mask1; // diagonal
			mask1 = mask1 >> 2;
			mask2 = mask2 << 2;
		}
	}
	template<int pad>
	void undo_move(matrix<uint32_t> &features, matrix<int> &board, const Move &move) noexcept
	{
		assert(board.isSquare());
		assert(move.sign != Sign::NONE);
		assert(board.at(pad + move.row, pad + move.col) == static_cast<int>(move.sign));

		board.at(pad + move.row, pad + move.col) = static_cast<int>(Sign::NONE);

		// first calculate rows above move
		uint32_t mask1 = ~(3 << (4 * pad));
		uint32_t mask2 = ~3;
		for (int i = 0; i <= pad; i++)
		{
			features.at(move.row + i, (pad + move.col - (pad - i)) * 4 + 2) &= mask1; // diagonal
			features.at(move.row + i, (pad + move.col) * 4 + 1) &= mask1; // vertical
			features.at(move.row + i, (pad + move.col + (pad - i)) * 4 + 3) &= mask2; // antidiagonal
			mask1 = mask1 >> 2;
			mask2 = (mask2 << 2) | 3;
		}

		uint32_t mask3 = ~(3 << (4 * pad));
		for (int i = 0; i <= 2 * pad; i++)
		{
			features.at(pad + move.row, (move.col + i) * 4) &= mask3; // horizontal
			mask3 = mask3 >> 2;
		}

		for (int i = 1; i <= pad; i++)
		{
			features.at(pad + move.row + i, (pad + move.col - i) * 4 + 3) &= mask2; // antidiagonal
			features.at(pad + move.row + i, (pad + move.col) * 4 + 1) &= mask1; // vertical
			features.at(pad + move.row + i, (pad + move.col + i) * 4 + 2) &= mask1; // diagonal
			mask1 = mask1 >> 2;
			mask2 = (mask2 << 2) | 3;
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

	std::pair<ThreatType, int> get_best_threat(ThreatType horizontal, ThreatType vertical, ThreatType diagonal, ThreatType antidiagonal)
	{
		std::pair<ThreatType, int> result(horizontal, static_cast<int>(Direction::HORIZONTAL));
		if (vertical > result.first)
			result = { vertical, static_cast<int>(Direction::VERTICAL) };
		if (diagonal > result.first)
			result = { diagonal, static_cast<int>(Direction::DIAGONAL) };
		if (antidiagonal > result.first)
			result = { antidiagonal, static_cast<int>(Direction::ANTIDIAGONAL) };
		return result;
	}
}

namespace ag
{

	FeatureExtractor::FeatureExtractor(GameConfig gameConfig) :
			nodes_buffer(10000),
			game_config(gameConfig),
			pad((game_config.rules == GameRules::FREESTYLE) ? 4 : 5),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			features(internal_board.rows(), internal_board.cols() * 4),
			cross_threats(internal_board.rows(), internal_board.cols() * 4),
			circle_threats(internal_board.rows(), internal_board.cols() * 4)
	{
		internal_board.fill(3);
		create_hashtable();
	}

	void FeatureExtractor::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		static_assert(sizeof(Sign) == sizeof(int));
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;
		for (int r = 0; r < board.rows(); r++)
			std::memcpy(internal_board.data(pad + r) + pad, board.data(r), sizeof(int) * board.cols());
		current_board_hash = get_hash();
		calc_all_features();
		get_threat_lists();
	}
	ProvenValue FeatureExtractor::solve(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList)
	{
		std::vector<Move> &own_five = (sign_to_move == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.empty() == false) // can make a five
		{
			for (auto iter = own_five.begin(); iter < own_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), 1.0f });
			return ProvenValue::LOSS; // it is instant win, returning inverted value
		}

		std::vector<Move> &opponent_five = (sign_to_move == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.empty() == false) // opponent can make a five
		{
			for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), 1.0f });
			if (opponent_five.size() == 1)
				return ProvenValue::UNKNOWN;
			else
				return ProvenValue::WIN; // opponent can have more that one five - loss in 2 plys
		}

		std::vector<Move> &own_open_four = (sign_to_move == Sign::CROSS) ? cross_open_four : circle_open_four;
		if (own_open_four.empty() == false) // can make an open four
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });
			return ProvenValue::LOSS; // it is win in 3 plys, returning inverted value;
		}

		std::vector<Move> &own_half_open_four = (sign_to_move == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
		std::vector<Move> &opponent_open_four = (sign_to_move == Sign::CROSS) ? circle_open_four : cross_open_four;
		std::vector<Move> &opponent_half_open_four = (sign_to_move == Sign::CROSS) ? circle_half_open_four : cross_half_open_four;
		if (opponent_open_four.empty() == false) // opponent can make an open four
		{
			for (auto iter = opponent_open_four.begin(); iter < opponent_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });
			for (auto iter = opponent_half_open_four.begin(); iter < opponent_half_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });

			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });
		}

		if (own_open_four.size() + own_half_open_four.size() == 0)
			return ProvenValue::UNKNOWN;

		if (use_caching and hashtable.load_factor() > 0.9)
			hashtable.clear();

		position_counter = 0;
		nodes_buffer.front().init(0, 0, invertSign(sign_to_move));
		node_counter = 1;

		recursive_solve(nodes_buffer.front(), false, 0);
		total_positions += position_counter;
		std::cout << "result = " << static_cast<int>(nodes_buffer.front().solved_value) << ", checked " << position_counter << " positions, total = "
				<< total_positions << "\n";
		switch (nodes_buffer.front().solved_value)
		{
			default:
			case SolvedValue::UNKNOWN:
			case SolvedValue::UNSOLVED:
				return ProvenValue::UNKNOWN;
			case SolvedValue::SOLVED_LOSS:
			{
				policy.clear();
				moveList.clear();
				for (auto iter = nodes_buffer[0].children; iter < nodes_buffer[0].children + nodes_buffer[0].number_of_children; iter++)
					if (iter->solved_value == SolvedValue::SOLVED_WIN)
					{
						policy.at(iter->move.row, iter->move.col) = 1.0f;
						moveList.push_back( { Move::move_to_short(iter->move.row, iter->move.col, sign_to_move), 1.0f });
					}
				return ProvenValue::LOSS;
			}
			case SolvedValue::SOLVED_WIN:
				return ProvenValue::WIN;
		}
		return ProvenValue::UNKNOWN; //nodes_buffer.front().solved_value;
	}

	void FeatureExtractor::printFeature(int row, int col) const
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
			std::cout << " : (" << features.at(pad + row, (pad + col) * 4 + i) << ")\n";
		}
	}
	void FeatureExtractor::printThreat(int row, int col) const
	{
		const FeatureTable &table = get_feature_table(game_config.rules);
		std::cout << "Threat for cross at  (" << row << ", " << col << ") = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(table.getThreat(get_feature_at(row, col, static_cast<Direction>(i))).for_cross) << ", ";
		std::cout << "in map = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(cross_threats.at(pad + row, (pad + col) * 4 + i)) << ", ";
		std::cout << '\n';

		std::cout << "Threat for circle at (" << row << ", " << col << ") = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(table.getThreat(get_feature_at(row, col, static_cast<Direction>(i))).for_circle) << ", ";
		std::cout << "in map = ";
		for (int i = 0; i < 4; i++)
			std::cout << ag::toString(circle_threats.at(pad + row, (pad + col) * 4 + i)) << ", ";
		std::cout << '\n';
	}
	void FeatureExtractor::printAllThreats() const
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
	void FeatureExtractor::print() const
	{
		std::cout << "hash = " << current_board_hash << '\n';
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

	void FeatureExtractor::addMove(Move move) noexcept
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
		current_board_hash = update_hash(current_board_hash, move);
	}
	void FeatureExtractor::undoMove(Move move) noexcept
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
		current_board_hash = update_hash(current_board_hash, move);
	}
// private
	uint64_t FeatureExtractor::get_hash() const noexcept
	{
		assert(static_cast<int>(hashing_keys.size()) == 2 * game_config.rows * game_config.cols);
		uint64_t result = 0;
		int i = 0;
		for (int row = 0; row < game_config.rows; row++)
			for (int col = 0; col < game_config.cols; col++, i += 2)
				switch (internal_board.at(pad + row, pad + col))
				{
					case 0:
						break;
					case 1:
						result ^= hashing_keys[i];
						break;
					case 2:
						result ^= hashing_keys[i + 1];
						break;
					case 3:
						break;
				}
		return result;
	}
	uint64_t FeatureExtractor::update_hash(uint64_t old_hash, Move move) const noexcept
	{
		assert(static_cast<int>(hashing_keys.size()) == 2 * game_config.rows * game_config.cols);
		return old_hash ^ hashing_keys[move.row * game_config.cols + move.col + static_cast<int>(move.sign) - 1];
	}
	void FeatureExtractor::create_hashtable()
	{
		for (int i = 0; i < 2 * game_config.rows * game_config.cols; i++)
			hashing_keys.push_back(randLong());
		if (use_caching)
			hashtable = std::unordered_map<uint64_t, SolvedValue, KeyHash, KeyCompare>(10 * max_positions);
	}
	uint32_t FeatureExtractor::get_feature_at(int row, int col, Direction dir) const noexcept
	{
		return features.at(pad + row, (pad + col) * 4 + static_cast<int>(dir));
	}
	void FeatureExtractor::calc_all_features() noexcept
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
	void FeatureExtractor::get_threat_lists()
	{
		cross_five.clear();
		cross_open_four.clear();
		cross_half_open_four.clear();
		circle_five.clear();
		circle_open_four.clear();
		circle_half_open_four.clear();

		const FeatureTable &table = get_feature_table(game_config.rules);
		for (int row = 0; row < internal_board.rows() - 2 * pad; row++)
			for (int col = 0; col < internal_board.cols() - 2 * pad; col++)
				if (internal_board.at(pad + row, pad + col) == 0)
				{
					ThreatType best_cross = ThreatType::NONE, best_circle = ThreatType::NONE;
					for (int i = 0; i < 4; i++)
					{
						Threat tmp = table.getThreat(get_feature_at(row, col, static_cast<Direction>(i)));
						cross_threats.at(pad + row, (pad + col) * 4 + i) = tmp.for_cross;
						circle_threats.at(pad + row, (pad + col) * 4 + i) = tmp.for_circle;
						best_cross = std::max(best_cross, tmp.for_cross);
						best_circle = std::max(best_circle, tmp.for_circle);
					}
//					Threat horizontal = table.getThreat(get_feature_at(row, col, Direction::HORIZONTAL));
//					Threat vertical = table.getThreat(get_feature_at(row, col, Direction::VERTICAL));
//					Threat diagonal = table.getThreat(get_feature_at(row, col, Direction::DIAGONAL));
//					Threat antidiagonal = table.getThreat(get_feature_at(row, col, Direction::ANTIDIAGONAL));
//
//					cross_threats.at(pad + row, (pad + col) * 4 + 0) = horizontal.for_cross;
//					cross_threats.at(pad + row, (pad + col) * 4 + 1) = vertical.for_cross;
//					cross_threats.at(pad + row, (pad + col) * 4 + 2) = diagonal.for_cross;
//					cross_threats.at(pad + row, (pad + col) * 4 + 3) = antidiagonal.for_cross;
//
//					circle_threats.at(pad + row, (pad + col) * 4 + 0) = horizontal.for_circle;
//					circle_threats.at(pad + row, (pad + col) * 4 + 1) = vertical.for_circle;
//					circle_threats.at(pad + row, (pad + col) * 4 + 2) = diagonal.for_circle;
//					circle_threats.at(pad + row, (pad + col) * 4 + 3) = antidiagonal.for_circle;

					switch (best_cross)
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

					switch (best_circle)
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
					for (int i = 0; i < 4; i++)
					{
						cross_threats.at(pad + row, (pad + col) * 4 + i) = ThreatType::NONE;
						circle_threats.at(pad + row, (pad + col) * 4 + i) = ThreatType::NONE;
					}
				}
	}
	void FeatureExtractor::recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth)
	{
		position_counter++;
		print();
		printAllThreats();

		Sign signToMove = invertSign(node.move.sign);
		std::vector<Move> &own_five = (signToMove == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.size() > 0)
		{
			node.solved_value = SolvedValue::SOLVED_LOSS;
			return; // if side to move can make a five, mark this node as loss
		}

		std::vector<Move> &opponent_five = (signToMove == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.size() > 1)
		{
			node.solved_value = SolvedValue::SOLVED_WIN;
			return; // if the other side side to move can make more than 1 five, mark this node as win
		}

		std::vector<Move> &own_open_four = (signToMove == Sign::CROSS) ? cross_open_four : circle_open_four;
		std::vector<Move> &own_half_open_four = (signToMove == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;

		if (opponent_five.size() > 0)
			node.number_of_children = 1; // there is only one defensive move
		else
			node.number_of_children = static_cast<int>(own_open_four.size() + own_half_open_four.size());

		node.solved_value = SolvedValue::UNSOLVED;
		if (node.number_of_children == 0)
			return; // no available threats
		if (node_counter + node.number_of_children >= static_cast<int>(nodes_buffer.size()))
			return; // no more nodes, cannot continue

		node.children = nodes_buffer.data() + node_counter;
		node_counter += node.number_of_children;

		if (opponent_five.size() > 0)
		{
			assert(opponent_five.size() == 1);
			node.children[0].init(opponent_five[0].row, opponent_five[0].col, signToMove);
			std::cout << node.children[0].move.toString() << " defensive move\n";
		}
		else
		{
			size_t children_counter = 0;
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++, children_counter++)
			{
				node.children[children_counter].init(iter->row, iter->col, signToMove);
				std::cout << node.children[children_counter].move.toString() << " open four\n";
			}
			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++, children_counter++)
			{
				node.children[children_counter].init(iter->row, iter->col, signToMove);
				std::cout << node.children[children_counter].move.toString() << " half open four\n";
			}
		}

		int loss_counter = 0;
		bool has_win_child = false;
		bool has_unproven_child = false;
		for (auto iter = node.children; iter < node.children + node.number_of_children; iter++)
		{
			if (iter != node.children)
			{
				print();
				printAllThreats();
			}
			std::cout << "depth = " << depth << " : state of all children:\n";
			for (int i = 0; i < node.number_of_children; i++)
				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';
			std::cout << "now checking " << iter->move.toString() << '\n';

			uint64_t new_hash = update_hash(current_board_hash, iter->move);
			auto cache_entry = use_caching ? hashtable.find(new_hash) : hashtable.end();
			if (cache_entry == hashtable.end())
			{
				if (position_counter < max_positions)
				{
					addMove(iter->move);
					recursive_solve(*iter, not mustProveAllChildren, depth + 1);
					undoMove(iter->move);
				}
			}
			else
				iter->solved_value = cache_entry->second; // cache hit

			if (iter->solved_value == SolvedValue::SOLVED_WIN) // found winning move, can skip remaining nodes
			{
				has_win_child = true;
				break;
			}
			else
			{
				if (iter->solved_value == SolvedValue::SOLVED_LOSS) // child node is losing
					loss_counter++;
				else // UNKNOWN or UNSOLVED
				{
					has_unproven_child = true;
					if (mustProveAllChildren)
						break;
				}
			}
		}

		std::cout << "depth = " << depth << " : state of all children:\n";
		for (int i = 0; i < node.number_of_children; i++)
			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';

		if (has_win_child)
			node.solved_value = SolvedValue::SOLVED_LOSS;
		else
		{
			if (has_unproven_child and mustProveAllChildren)
				node.solved_value = SolvedValue::UNSOLVED;
			else
			{
				if (loss_counter == node.number_of_children)
					node.solved_value = SolvedValue::SOLVED_WIN;
				else
					node.solved_value = SolvedValue::UNSOLVED;
			}
		}
		if (use_caching)
			hashtable.insert(std::pair<uint64_t, SolvedValue>(current_board_hash, node.solved_value));
		node_counter -= node.number_of_children;
	}
	std::string FeatureExtractor::toString(SolvedValue sv)
	{
		switch (sv)
		{
			default:
			case SolvedValue::UNKNOWN:
				return "UNKNOWN";
			case SolvedValue::UNSOLVED:
				return "UNSOLVED";
			case SolvedValue::SOLVED_LOSS:
				return "SOLVED_LOSS";
			case SolvedValue::SOLVED_WIN:
				return "SOLVED_WIN";
		}
	}

	void FeatureExtractor::update_threats(int row, int col)
	{
		const FeatureTable &table = get_feature_table(game_config.rules);
		for (int i = 0; i <= pad; i++)
		{
			update_threat_at(table, row - pad + i, col - pad + i, Direction::DIAGONAL);
			update_threat_at(table, row - pad + i, col, Direction::VERTICAL);
			update_threat_at(table, row - pad + i, col + pad - i, Direction::ANTIDIAGONAL);
		}
		for (int i = col - pad; i <= col+pad; i++)
			update_threat_at(table, row, i, Direction::HORIZONTAL);
		for (int i = 1; i <= pad; i++)
		{
			update_threat_at(table, row + i, col - i, Direction::ANTIDIAGONAL);
			update_threat_at(table, row + i, col, Direction::VERTICAL);
			update_threat_at(table, row + i, col + i, Direction::DIAGONAL);
		}
	}
	void FeatureExtractor::update_threat_at(const FeatureTable &table, int row, int col, Direction direction)
	{
		Threat t = (internal_board.at(pad + row, pad + col) == 0) ? table.getThreat(get_feature_at(row, col, direction)) : Threat();
		update_threat_at(t.for_cross, row, col, cross_five, cross_open_four, cross_half_open_four, cross_threats, static_cast<int>(direction));
		update_threat_at(t.for_circle, row, col, circle_five, circle_open_four, circle_half_open_four, circle_threats, static_cast<int>(direction));
	}
	void FeatureExtractor::update_threat_at(ThreatType new_threat, int row, int col, std::vector<Move> &five, std::vector<Move> &open_four,
			std::vector<Move> &half_open_four, matrix<ThreatType> &threats, int direction)
	{
		ThreatType old_best_threat = get_best_threat_at(threats, row, col);
		threats.at(pad + row, (pad + col) * 4 + direction) = new_threat;
		ThreatType new_best_threat = get_best_threat_at(threats, row, col);

//		std::cout << "updating at " << row << "," << col << " dir=" << direction << " with " << ag::toString(new_best_threat)
//				<< ", previously " + ag::toString(old_best_threat) << '\n';
		if (new_best_threat == old_best_threat)
			return; // the same threat as previously

//		std::cout << "\n";
//		printFeature(row, col);
//		printThreat(row, col);
//		std::cout << "--------threats before\n";
//		printAllThreats();
		switch (old_best_threat)
		{
			case ThreatType::NONE:
				break;
			case ThreatType::HALF_OPEN_FOUR:
			{
//				std::cout << "removing " << row << "," << col << " from half open fours\n";
				auto index = std::find(half_open_four.begin(), half_open_four.end(), Move(row, col));
				assert(index != half_open_four.end());
				half_open_four.erase(index);
				break;
			}
			case ThreatType::OPEN_FOUR:
			{
//				std::cout << "removing " << row << "," << col << " from open fours\n";
				auto index = std::find(open_four.begin(), open_four.end(), Move(row, col));
				assert(index != open_four.end());
				open_four.erase(index);
				break;
			}
			case ThreatType::FIVE:
			{
//				std::cout << "removing " << row << "," << col << " from fives\n";
				auto index = std::find(five.begin(), five.end(), Move(row, col));
				assert(index != five.end());
				five.erase(index);
				break;
			}
		}

		auto compare_moves = [](Move lhs, Move rhs)
		{	return lhs.row * 32 + lhs.col < rhs.row * 32 + rhs.col;};
//		std::cout << "\n--------threats after removing\n";
//		printAllThreats();
		switch (new_best_threat)
		{
			case ThreatType::NONE:
				break;
			case ThreatType::HALF_OPEN_FOUR:
				half_open_four.push_back(Move(row, col));
				std::sort(half_open_four.begin(), half_open_four.end(), compare_moves);
				break;
			case ThreatType::OPEN_FOUR:
				open_four.push_back(Move(row, col));
				std::sort(open_four.begin(), open_four.end(), compare_moves);
				break;
			case ThreatType::FIVE:
				five.push_back(Move(row, col));
				std::sort(five.begin(), five.end(), compare_moves);
				break;
		}

//		std::cout << "\n--------threats after adding\n";
//		printAllThreats();
//		std::cout << "---------------------------\n";
//		printFeature(row, col);
//		printThreat(row, col);
	}
	ThreatType FeatureExtractor::get_best_threat_at(const matrix<ThreatType> &threats, int row, int col) const noexcept
	{
		return std::max(std::max(threats.at(pad + row, (pad + col) * 4 + 0), threats.at(pad + row, (pad + col) * 4 + 1)),
				std::max(threats.at(pad + row, (pad + col) * 4 + 2), threats.at(pad + row, (pad + col) * 4 + 3)));
	}
} /* namespace ag */

