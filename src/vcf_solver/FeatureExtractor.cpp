/*
 * FeatureExtractor.cpp
 *
 *  Created on: May 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
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
				line = line | (board.at(pad + i - j, pad + j) << ((pad + j) * 2));
			features.at(pad + i, pad * 4 + 3) = line;
			for (int j = 1; j <= i; j++)
			{
				line = (board.at(pad + i - j - pad, pad + j + pad) << shift) | (line >> 2);
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
			mask1 = (3 << 30) | (mask1 >> 2);
			mask2 = (mask2 << 2) | 3;
		}

		uint32_t mask3 = ~(3 << (4 * pad));
		for (int i = 0; i <= 2 * pad; i++)
		{
			features.at(pad + move.row, (move.col + i) * 4) &= mask3; // horizontal
			mask3 = (3 << 30) | (mask3 >> 2);
		}

		for (int i = 1; i <= pad; i++)
		{
			features.at(pad + move.row + i, (pad + move.col - i) * 4 + 3) &= mask2; // antidiagonal
			features.at(pad + move.row + i, (pad + move.col) * 4 + 1) &= mask1; // vertical
			features.at(pad + move.row + i, (pad + move.col + i) * 4 + 2) &= mask1; // diagonal
			mask1 = (3 << 30) | (mask1 >> 2);
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
}

namespace ag
{

	FeatureExtractor::FeatureExtractor(GameConfig gameConfig, int maxPositions) :
			statistics(gameConfig.rows * gameConfig.cols),
			max_positions(maxPositions),
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
//		static_assert(sizeof(Sign) == sizeof(int)); FIXME
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		sign_to_move = signToMove;
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
				internal_board.at(pad + row, pad + col) = static_cast<int>(board.at(row, col));
//		for (int r = 0; r < board.rows(); r++)
//			std::memcpy(internal_board.data(pad + r) + pad, board.data(r), sizeof(int) * board.cols());
		current_board_hash = get_hash();
		calc_all_features();
		get_threat_lists();
		root_depth = board.rows() * board.cols() - std::count(board.begin(), board.end(), Sign::NONE);
	}
	void FeatureExtractor::solve(SearchTask &task, int level)
	{
		setBoard(task.getBoard(), task.getSignToMove());

		std::vector<Move> &own_five = (sign_to_move == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.size() > 0) // can make a five
		{
			for (auto iter = own_five.begin(); iter < own_five.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.markAsReady();
			return; // it is a win in 1 ply
		}
		if (root_depth + 1 == game_config.rows * game_config.cols) // this is the last move before the board is full
		{
			Move m;
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
					if (internal_board.at(pad + row, pad + col) == 0)
						m = Move(sign_to_move, row, col);
			task.addProvenEdge(m, ProvenValue::DRAW);
			task.setValue(Value(0.0f, 1.0, 0.0f));
			task.markAsReady();
			return; // it is a draw
		}
		if (level <= 0)
			return; // only winning or draw moves will be found

		std::vector<Move> &opponent_five = (sign_to_move == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.size() > 0) // opponent can make a five
		{
			if (opponent_five.size() > 1)
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::LOSS);
				task.setValue(Value(0.0f, 0.0, 1.0f));
				task.markAsReady(); // the state is provably losing, there is no need to further evaluate it
			}
			else
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);
			}
			return;
		}

		std::vector<Move> &own_open_four = (sign_to_move == Sign::CROSS) ? cross_open_four : circle_open_four;
		if (own_open_four.size() > 0) // can make an open four, but it was already checked that opponent cannot make any five
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::WIN); // it is a win in 3 plys
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.markAsReady();
			return;
		}

		std::vector<Move> &own_half_open_four = (sign_to_move == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
		std::vector<Move> &opponent_open_four = (sign_to_move == Sign::CROSS) ? circle_open_four : cross_open_four;
		std::vector<Move> &opponent_half_open_four = (sign_to_move == Sign::CROSS) ? circle_half_open_four : cross_half_open_four;
		if (opponent_open_four.size() > 0) // opponent can make an open four, but no fives. We also cannot make any five or open four
		{
			for (auto iter = opponent_open_four.begin(); iter < opponent_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);
			for (auto iter = opponent_half_open_four.begin(); iter < opponent_half_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);

			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++)
			{
				Move move_to_be_added(sign_to_move, *iter);
				bool is_already_added = std::any_of(task.getProvenEdges().begin(), task.getProvenEdges().end(), [move_to_be_added](const Edge &edge)
				{	return edge.getMove() == move_to_be_added;}); // find if such move has been added in any of two loops above
				if (not is_already_added) // move must not be added twice
					task.addProvenEdge(move_to_be_added, ProvenValue::UNKNOWN);
			}
			return;
		}

		if (level <= 1)
			return; // winning and forced moves will be found

		if (own_open_four.size() + own_half_open_four.size() == 0)
			return; // there are not threats that we can make

		if (use_caching)
			hashtable.clear();

		position_counter = 0;
		node_counter = 1; // prepare node stack
		nodes_buffer.front().init(0, 0, invertSign(sign_to_move)); // prepare node stack
		recursive_solve(nodes_buffer.front(), false, 0);

		total_positions += position_counter;
		statistics[root_depth].add(nodes_buffer.front().solved_value == SolvedValue::SOLVED_LOSS, position_counter);
//		std::cout << "depth = " << root_depth << ", result = " << static_cast<int>(nodes_buffer.front().solved_value) << ", checked "
//				<< position_counter << " positions, total = " << total_positions << ", in cache " << hashtable.size() << "\n";
		if (nodes_buffer.front().solved_value == SolvedValue::SOLVED_LOSS)
		{
			for (auto iter = nodes_buffer[0].children; iter < nodes_buffer[0].children + nodes_buffer[0].number_of_children; iter++)
				if (iter->solved_value == SolvedValue::SOLVED_WIN)
					task.addProvenEdge(Move(sign_to_move, iter->move), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.markAsReady();
		}
	}
	ProvenValue FeatureExtractor::solve(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList)
	{
		std::vector<Move> &own_five = (sign_to_move == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.size() > 0) // can make a five
		{
			for (auto iter = own_five.begin(); iter < own_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), 1.0f });
			return ProvenValue::LOSS; // it is instant win, returning inverted value
		}
		std::vector<Move> &opponent_five = (sign_to_move == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.size() > 0) // opponent can make a five
		{
			for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), 1.0f });
			if (opponent_five.size() == 1)
				return ProvenValue::UNKNOWN;
			else
				return ProvenValue::WIN; // opponent can have more that one five - loss in 2 plys
		}

		std::vector<Move> &own_open_four = (sign_to_move == Sign::CROSS) ? cross_open_four : circle_open_four;
		if (own_open_four.size() > 0) // can make an open four
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });
			return ProvenValue::LOSS; // it is win in 3 plys, returning inverted value;
		}

		std::vector<Move> &own_half_open_four = (sign_to_move == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
		std::vector<Move> &opponent_open_four = (sign_to_move == Sign::CROSS) ? circle_open_four : cross_open_four;
		std::vector<Move> &opponent_half_open_four = (sign_to_move == Sign::CROSS) ? circle_half_open_four : cross_half_open_four;
		if (opponent_open_four.size() > 0) // opponent can make an open four
		{
			for (auto iter = opponent_open_four.begin(); iter < opponent_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });
			for (auto iter = opponent_half_open_four.begin(); iter < opponent_half_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, sign_to_move), policy.at(iter->row, iter->col) });

			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++)
			{
				uint16_t new_move = Move::move_to_short(iter->row, iter->col, sign_to_move);
				bool is_already_added = std::any_of(moveList.begin(), moveList.end(), [new_move](auto e)
				{	return e.first == new_move;}); // find if such move has been added in any of two loops above
				if (not is_already_added) // move must not be added twice
					moveList.push_back( { new_move, policy.at(iter->row, iter->col) });
			}
		}

		if (own_open_four.size() + own_half_open_four.size() == 0)
			return ProvenValue::UNKNOWN;

		if (use_caching)
			hashtable.clear();

		position_counter = 0;
		node_counter = 1; // prepare node stack
		nodes_buffer.front().init(0, 0, invertSign(sign_to_move)); // prepare node stack
		recursive_solve(nodes_buffer.front(), false, 0);

		total_positions += position_counter;
		statistics[root_depth].add(nodes_buffer.front().solved_value == SolvedValue::SOLVED_LOSS, position_counter);
//		std::cout << "depth = " << root_depth << ", result = " << static_cast<int>(nodes_buffer.front().solved_value) << ", checked "
//				<< position_counter << " positions, total = " << total_positions << ", in cache " << hashtable.size() << "\n";
		if (nodes_buffer.front().solved_value == SolvedValue::SOLVED_LOSS)
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
		else
			return ProvenValue::UNKNOWN;
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

	uint32_t FeatureExtractor::getFeatureAt(int row, int col, Direction dir) const noexcept
	{
		return features.at(pad + row, 4 * (pad + col) + static_cast<int>(dir));
	}
	ThreatType FeatureExtractor::getThreatAt(Sign sign, int row, int col, Direction dir) const noexcept
	{
		switch (sign)
		{
			default:
			case Sign::NONE:
				return ThreatType::NONE;
			case Sign::CROSS:
				return cross_threats.at(pad + row, 4 * (pad + col) + static_cast<int>(dir));
			case Sign::CIRCLE:
				return circle_threats.at(pad + row, 4 * (pad + col) + static_cast<int>(dir));
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

	void FeatureExtractor::print_stats() const
	{
		for (size_t i = 0; i < statistics.size(); i++)
			if (statistics[i].calls > 0)
			{
				std::cout << "depth = " << i << " : " << statistics[i].hits << "/" << statistics[i].calls << ", positions "
						<< statistics[i].positions_hit << "/" << (statistics[i].positions_hit + statistics[i].positions_miss) << std::endl;
			}
	}

	/*
	 * private
	 */
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
		return old_hash ^ hashing_keys[2 * (move.row * game_config.cols + move.col) + static_cast<int>(move.sign) - 1];
	}
	void FeatureExtractor::create_hashtable()
	{
		for (int i = 0; i < 2 * game_config.rows * game_config.cols; i++)
			hashing_keys.push_back(randLong());
		if (use_caching)
			hashtable = std::unordered_map<uint64_t, SolvedValue, KeyHash, KeyCompare>(cache_size);
	}
	uint32_t FeatureExtractor::get_feature_at(int row, int col, Direction dir) const noexcept
	{
		return features.at(pad + row, (pad + col) * 4 + static_cast<int>(dir));
	}
	void FeatureExtractor::calc_all_features() noexcept
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
					ThreatType best_cross = ThreatType::NONE;
					ThreatType best_circle = ThreatType::NONE;
					for (int dir = 0; dir < 4; dir++)
					{
						Threat tmp = table.getThreat(get_feature_at(row, col, static_cast<Direction>(dir)));
						cross_threats.at(pad + row, (pad + col) * 4 + dir) = tmp.for_cross;
						circle_threats.at(pad + row, (pad + col) * 4 + dir) = tmp.for_circle;
						best_cross = std::max(best_cross, tmp.for_cross);
						best_circle = std::max(best_circle, tmp.for_circle);
					}

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
					for (int dir = 0; dir < 4; dir++)
					{
						cross_threats.at(pad + row, (pad + col) * 4 + dir) = ThreatType::NONE;
						circle_threats.at(pad + row, (pad + col) * 4 + dir) = ThreatType::NONE;
					}
				}
	}
	void FeatureExtractor::recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth)
	{
		position_counter++;
//		print();
//		printAllThreats();

		Sign signToMove = invertSign(node.move.sign);
		const std::vector<Move> &own_five = (signToMove == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.size() > 0) // if side to move can make a five, mark this node as loss
		{
			node.solved_value = SolvedValue::SOLVED_LOSS;
			return;
		}

		const std::vector<Move> &opponent_five = (signToMove == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.size() > 1) // if the other side side to move can make more than one five, mark this node as win
		{
			node.solved_value = SolvedValue::SOLVED_WIN;
			return;
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
			return; // no more nodes in buffer, cannot continue

		node.children = nodes_buffer.data() + node_counter;
		node_counter += node.number_of_children;

		if (opponent_five.size() > 0) // must make this defensive move
		{
			assert(opponent_five.size() == 1); // it was checked earlier that opponent cannot have more than one five
			node.children[0].init(opponent_five[0].row, opponent_five[0].col, signToMove);
		}
		else
		{
//			std::random_shuffle(own_open_four.begin(), own_open_four.end());
//			std::random_shuffle(own_half_open_four.begin(), own_half_open_four.end());
			size_t children_counter = 0;
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++, children_counter++)
				node.children[children_counter].init(iter->row, iter->col, signToMove);
			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++, children_counter++)
				node.children[children_counter].init(iter->row, iter->col, signToMove);
		}

		int loss_counter = 0;
		bool has_win_child = false;
		bool has_unproven_child = false;
		for (auto iter = node.children; iter < node.children + node.number_of_children; iter++)
		{
//			if (iter != node.children)
//			{
//				print();
//				printAllThreats();
//			}
//			std::cout << "depth = " << depth << " : state of all children:\n";
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';
//			std::cout << "now checking " << iter->move.toString() << '\n';

			uint64_t new_hash = update_hash(current_board_hash, iter->move);
			auto cache_entry = use_caching ? hashtable.find(new_hash) : hashtable.end();
			if (cache_entry == hashtable.end())
			{
				if (position_counter < max_positions and depth <= max_depth)
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
					if (mustProveAllChildren) // if this is defensive side, it must prove all its child nodes as losing
						break; // so the function can exit now, as there will be at least one unproven node (current one)
				}
			}
		}

//		std::cout << "depth = " << depth << " : state of all children:\n";
//		for (int i = 0; i < node.number_of_children; i++)
//			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';

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
	void FeatureExtractor::update_threat_at(const FeatureTable &table, int row, int col, Direction direction)
	{
		if (row >= 0 and row < game_config.rows and col >= 0 and col < game_config.cols) // only inside board
		{
			// check what was the best threat at (row, col) before updating
			Threat old_best_threat(get_best_threat_at(cross_threats, row, col), get_best_threat_at(circle_threats, row, col));

			// update the threat tables with newly formed threat in the given direction
			Threat new_threat = table.getThreat(get_feature_at(row, col, direction)); // this is new threat as taken from feature table
			cross_threats.at(pad + row, (pad + col) * 4 + static_cast<int>(direction)) = new_threat.for_cross;
			circle_threats.at(pad + row, (pad + col) * 4 + static_cast<int>(direction)) = new_threat.for_circle;

			// check what is the new best threat at (row, col) after threat tables was updated
			Threat new_best_threat(get_best_threat_at(cross_threats, row, col), get_best_threat_at(circle_threats, row, col));

			// update list of threats for cross and circle
			update_threat_list(old_best_threat.for_cross, new_best_threat.for_cross, Move(row, col), cross_five, cross_open_four,
					cross_half_open_four);
			update_threat_list(old_best_threat.for_circle, new_best_threat.for_circle, Move(row, col), circle_five, circle_open_four,
					circle_half_open_four);
		}
	}
	void FeatureExtractor::update_threat_list(ThreatType old_threat, ThreatType new_threat, Move move, std::vector<Move> &five,
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
				open_four.erase(index);
				break;
			}
			case ThreatType::FIVE:
			{
				auto index = std::find(five.begin(), five.end(), move);
				assert(index != five.end());
				five.erase(index);
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
	ThreatType FeatureExtractor::get_best_threat_at(const matrix<ThreatType> &threats, int row, int col) const noexcept
	{
		return std::max(std::max(threats.at(pad + row, (pad + col) * 4 + 0), threats.at(pad + row, (pad + col) * 4 + 1)),
				std::max(threats.at(pad + row, (pad + col) * 4 + 2), threats.at(pad + row, (pad + col) * 4 + 3)));
	}

} /* namespace ag */

