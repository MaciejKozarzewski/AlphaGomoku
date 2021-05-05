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
	void horizontal(matrix<uint32_t> &features, const matrix<int> &board)
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
	void vertical(matrix<uint32_t> &features, const matrix<int> &board)
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
	void diagonal(matrix<uint32_t> &features, const matrix<int> &board)
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
	void antidiagonal(matrix<uint32_t> &features, const matrix<int> &board)
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
	void add_move(matrix<uint32_t> &features, matrix<int> &board, const Move &move)
	{
		assert(board.isSquare());
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
	void undo_move(matrix<uint32_t> &features, matrix<int> &board, const Move &move)
	{
		assert(board.isSquare());
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
}

namespace ag
{

	FeatureExtractor::FeatureExtractor(GameConfig gameConfig) :
			nodes_buffer(10000),
			rules(gameConfig.rules),
			pad((rules == GameRules::FREESTYLE) ? 4 : 5),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			features(internal_board.rows(), internal_board.cols() * 4),
			cross_threats(internal_board.rows(), internal_board.cols()),
			circle_threats(internal_board.rows(), internal_board.cols())
	{
		internal_board.fill(3);
	}

	ProvenValue FeatureExtractor::generateMoves(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList, const matrix<Sign> &board,
			Sign signToMove)
	{
		static_assert(sizeof(Sign) == sizeof(int));
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		for (int r = 0; r < board.rows(); r++)
			std::memcpy(internal_board.data(pad + r) + pad, board.data(r), sizeof(int) * board.cols());

		calc_all_features();
		get_threat_lists();

		std::vector<Move> &own_five = (signToMove == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.empty() == false) // can make a five
		{
			for (auto iter = own_five.begin(); iter < own_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), 1.0f });
			return ProvenValue::LOSS; // it is instant win, returning inverted value
		}

		std::vector<Move> &opponent_five = (signToMove == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.empty() == false) // opponent can make a five
		{
			for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), 1.0f });
			if (opponent_five.size() == 1)
				return ProvenValue::UNKNOWN;
			else
				return ProvenValue::WIN; // opponent can have more that one five - loss in 2 plys
		}

		std::vector<Move> &own_open_four = (signToMove == Sign::CROSS) ? cross_open_four : circle_open_four;
		if (own_open_four.empty() == false) // can make an open four
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), policy.at(iter->row, iter->col) });
			return ProvenValue::LOSS; // it is win in 3 plys, returning inverted value;
		}

		std::vector<Move> &own_half_open_four = (signToMove == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;
		std::vector<Move> &opponent_open_four = (signToMove == Sign::CROSS) ? circle_open_four : cross_open_four;
		std::vector<Move> &opponent_half_open_four = (signToMove == Sign::CROSS) ? circle_half_open_four : cross_half_open_four;
		if (opponent_open_four.empty() == false) // opponent can make an open four
		{
			for (auto iter = opponent_open_four.begin(); iter < opponent_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), policy.at(iter->row, iter->col) });
			for (auto iter = opponent_half_open_four.begin(); iter < opponent_half_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), policy.at(iter->row, iter->col) });

			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++)
				moveList.push_back( { Move::move_to_short(iter->row, iter->col, signToMove), policy.at(iter->row, iter->col) });
		}

		position_counter = 0;
		nodes_buffer.front().init(0, 0, invertSign(signToMove));
		node_counter = 1;
		solve(nodes_buffer.front(), false, 0);
//		std::cout << "checked " << position_counter << " positions\n";
		switch (nodes_buffer.front().solved_value)
		{
			case SolvedValue::UNKNOWN:
			case SolvedValue::UNSOLVED:
				return ProvenValue::UNKNOWN;
			case SolvedValue::SOLVED_LOSS:
			{
				policy.clear();
				for (auto iter = nodes_buffer[0].children; iter < nodes_buffer[0].children + nodes_buffer[0].number_of_children; iter++)
					if (iter->solved_value == SolvedValue::SOLVED_WIN)
						policy.at(iter->move.row, iter->move.col) = 1.0f;
				return ProvenValue::LOSS;
			}
			case SolvedValue::SOLVED_WIN:
				return ProvenValue::WIN;
		}
		return ProvenValue::UNKNOWN; //nodes_buffer.front().solved_value;
	}
	void FeatureExtractor::printFeature(int x, int y)
	{
		std::cout << "Features at (" << x << ", " << y << ")" << std::endl;
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

			uint32_t line = get_feature_at(x, y, static_cast<Direction>(i));
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
			std::cout << " : (" << features.at(pad + x, (pad + y) * 4 + i) << ")\n";
		}
	}
	void FeatureExtractor::print()
	{
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

	void FeatureExtractor::setBoard(const matrix<Sign> &board)
	{
		static_assert(sizeof(Sign) == sizeof(int));
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		for (int r = 0; r < board.rows(); r++)
			std::memcpy(internal_board.data(pad + r) + pad, board.data(r), sizeof(int) * board.cols());
		calc_all_features();
		get_threat_lists();
	}
	void FeatureExtractor::addMove(Move move)
	{
		switch (rules)
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
				throw std::logic_error("rule not supported");
		}
		get_threat_lists();
	}
	void FeatureExtractor::undoMove(Move move)
	{
		switch (rules)
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
				throw std::logic_error("rule not supported");
		}
		get_threat_lists();
	}
	// private
	uint32_t FeatureExtractor::get_feature_at(int row, int col, Direction dir)
	{
		return features.at(pad + row, (pad + col) * 4 + static_cast<int>(dir));
	}
	void FeatureExtractor::calc_all_features()
	{
		switch (rules)
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
				throw std::logic_error("rule not supported");
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

		const FeatureTable &table = get_feature_table(rules);
		for (int row = 0; row < internal_board.rows() - 2 * pad; row++)
			for (int col = 0; col < internal_board.cols() - 2 * pad; col++)
				if (internal_board.at(pad + row, pad + col) == 0)
				{
					Threat horizontal = table.getThreat(get_feature_at(row, col, Direction::HORIZONTAL));
					Threat vertical = table.getThreat(get_feature_at(row, col, Direction::VERTICAL));
					Threat diagonal = table.getThreat(get_feature_at(row, col, Direction::DIAGONAL));
					Threat antidiagonal = table.getThreat(get_feature_at(row, col, Direction::ANTIDIAGONAL));

					ThreatType best_cross = std::max(std::max(horizontal.for_cross, vertical.for_cross),
							std::max(diagonal.for_cross, antidiagonal.for_cross));
					ThreatType best_circle = std::max(std::max(horizontal.for_circle, vertical.for_circle),
							std::max(diagonal.for_circle, antidiagonal.for_circle));
					cross_threats.at(pad + row, pad + col) = best_cross;
					circle_threats.at(pad + row, pad + col) = best_circle;

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
	}
	void FeatureExtractor::solve(InternalNode &node, bool mustProveAllChildren, int depth)
	{
		position_counter++;

		Sign sign_to_move = invertSign(node.move.sign);
		std::vector<Move> &own_five = (sign_to_move == Sign::CROSS) ? cross_five : circle_five;
		if (own_five.size() > 0)
		{
			node.solved_value = SolvedValue::SOLVED_LOSS;
			return; // if side to move can make a five, mark this node as loss
		}

		std::vector<Move> &opponent_five = (sign_to_move == Sign::CROSS) ? circle_five : cross_five;
		if (opponent_five.size() > 1)
		{
			node.solved_value = SolvedValue::SOLVED_WIN;
			return; // if the other side side to move can make more than 1 five, mark this node as win
		}

		std::vector<Move> &own_open_four = (sign_to_move == Sign::CROSS) ? cross_open_four : circle_open_four;
		std::vector<Move> &own_half_open_four = (sign_to_move == Sign::CROSS) ? cross_half_open_four : circle_half_open_four;

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

//		std::cout << "cross  : fives=" << cross_five.size() << ", open fours=" << cross_open_four.size() << ", half open fours="
//				<< cross_half_open_four.size() << '\n';
//		std::cout << "circle : fives=" << circle_five.size() << ", open fours=" << circle_open_four.size() << ", half open fours="
//				<< circle_half_open_four.size() << '\n';

		if (opponent_five.size() > 0)
		{
			assert(circle_five.size() == 1);
			node.children[0].init(opponent_five[0].row, opponent_five[0].col, sign_to_move);
//			std::cout << node.children[0].move.toString() << " defensive move\n";
		}
		else
		{
			size_t children_counter = 0;
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++, children_counter++)
			{
				node.children[children_counter].init(iter->row, iter->col, sign_to_move);
//				std::cout << node.children[children_counter].move.toString() << " open four\n";
			}
			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++, children_counter++)
			{
				node.children[children_counter].init(iter->row, iter->col, sign_to_move);
//				std::cout << node.children[children_counter].move.toString() << " half open four\n";
			}
		}

		std::vector<Move> &opponent_open_four = (sign_to_move == Sign::CROSS) ? circle_open_four : cross_open_four;
		std::vector<Move> &opponent_half_open_four = (sign_to_move == Sign::CROSS) ? circle_half_open_four : cross_half_open_four;

		int loss_counter = 0;
		bool has_win_child = false;
		bool has_unproven_child = false;
		for (auto iter = node.children; iter < node.children + node.number_of_children; iter++)
		{
//			std::cout << "depth = " << depth << " : state of all children:\n";
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';
//			std::cout << "now checking " << iter->move.toString() << '\n';

//			internal_board.at(pad + iter->move.row, pad + iter->move.col) = static_cast<int>(iter->move.sign);
			if (position_counter < max_positions)
			{
				addMove(iter->move);
				if (opponent_five.size() + opponent_open_four.size() + opponent_half_open_four.size() > 0)
					solve(*iter, not mustProveAllChildren, depth + 1);
				undoMove(iter->move);
			}
//			internal_board.at(pad + iter->move.row, pad + iter->move.col) = 0;

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
		node_counter -= node.number_of_children;
	}
	void FeatureExtractor::update_threats(int row, int col)
	{
		const FeatureTable &table = get_feature_table(rules);

		for (int i = 0; i <= pad; i++)
		{
//			int c = col + i;
//			if (internal_board.at(row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::DIAGONAL));
//				if (t.for_cross == ThreatType::NONE)
//				{
//					cross_threats.at(row + i, c) = ThreatType::NONE;
//				}
//				if (map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::DIAGONAL)))
//				{
//
//				}
//				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::DIAGONAL)) = t;
//			}
//			c = pad + col;
//			if (internal_board.at(row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::VERTICAL));
//				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::VERTICAL)) = t;
//			}
//			c = 2 * pad + col - i;
//			if (internal_board.at(row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::ANTIDIAGONAL));
//				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::ANTIDIAGONAL)) = t;
//			}
		}

//		for (int i = pad + col; i <= col + 2 * pad; i++)
//			if (internal_board.at(row + i, col) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(row + i, col, Direction::HORIZONTAL));
//				map_of_threats.at(row + i, col * 4 + static_cast<int>(Direction::HORIZONTAL)) = t;
//			}

		for (int i = 1; i <= pad; i++)
		{
//			int c = pad + col - i;
//			if (internal_board.at(pad + row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::ANTIDIAGONAL));
//				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::ANTIDIAGONAL)) = t;
//			}
//			c = pad + col;
//			if (internal_board.at(pad + row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::VERTICAL));
//				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::VERTICAL)) = t;
//			}
//			c = pad + col + i;
//			if (internal_board.at(pad + row + i, c) == 0)
//			{
//				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::DIAGONAL));
//				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::DIAGONAL)) = t;
//			}
		}
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
} /* namespace ag */

