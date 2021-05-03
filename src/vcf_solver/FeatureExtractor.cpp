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
		assert(board.at(move.row, move.col) == 0); // move must be made on empty spot
		board.at(pad + move.row, pad + move.col) = static_cast<int>(move.sign);

		// first calculate rows above move
		uint32_t mask = static_cast<uint32_t>(move.sign) << (4 * pad);
		for (int i = 0; i <= pad; i++)
		{
			features.at(move.row + i, (pad + move.col - (pad - i)) * 4 + 2) |= mask;
			features.at(move.row + i, (pad + move.col) * 4 + 1) |= mask;
			features.at(move.row + i, (pad + move.col + (pad - i)) * 4 + 3) |= mask;
			mask = mask >> 2;
		}

		mask = static_cast<uint32_t>(move.sign) << (4 * pad);
		for (int i = pad + move.col; i <= move.col + 2 * pad; i++)
		{
			features.at(pad + move.row, i * 4) |= mask;
			mask = mask >> 2;
		}

		mask = static_cast<uint32_t>(move.sign) << (2 * pad - 2);
		for (int i = 1; i <= pad; i++)
		{
			features.at(pad + move.row + i, (pad + move.col - i) * 4 + 3) |= mask;
			features.at(pad + move.row + i, (pad + move.col) * 4 + 1) |= mask;
			features.at(pad + move.row + i, (pad + move.col + i) * 4 + 2) |= mask;
			mask = mask >> 2;
		}
	}
	template<int pad>
	void delete_move(matrix<uint32_t> &features, matrix<int> &board, const Move &move)
	{
		assert(board.isSquare());
		assert(board.at(move.row, move.col) == static_cast<int>(move.sign));
		board.at(pad + move.row, pad + move.col) = static_cast<int>(Sign::NONE);

		// first calculate rows above move
		uint32_t mask = ~(static_cast<uint32_t>(move.sign) << (4 * pad));
		for (int i = 0; i <= pad; i++)
		{
			features.at(move.row + i, (pad + move.col - (pad - i)) * 4 + 2) &= mask;
			features.at(move.row + i, (pad + move.col) * 4 + 1) &= mask;
			features.at(move.row + i, (pad + move.col + (pad - i)) * 4 + 3) &= mask;
			mask = mask >> 2;
		}

		mask = ~(static_cast<uint32_t>(move.sign) << (4 * pad));
		for (int i = pad + move.col; i <= move.col + 2 * pad; i++)
		{
			features.at(pad + move.row, i * 4) &= mask;
			mask = mask >> 2;
		}

		mask = ~(static_cast<uint32_t>(move.sign) << (2 * pad - 2));
		for (int i = 1; i <= pad; i++)
		{
			features.at(pad + move.row + i, (pad + move.col - i) * 4 + 3) &= mask;
			features.at(pad + move.row + i, (pad + move.col) * 4 + 1) &= mask;
			features.at(pad + move.row + i, (pad + move.col + i) * 4 + 2) &= mask;
			mask = mask >> 2;
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
			rules(gameConfig.rules),
			pad((rules == GameRules::FREESTYLE) ? 4 : 5),
			internal_board(gameConfig.rows + 2 * pad, gameConfig.cols + 2 * pad),
			features(internal_board.rows(), internal_board.cols() * 4),
			map_of_threats(internal_board.rows(), internal_board.cols() * 4)
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
		get_threat_lists(signToMove);

		auto compare = [&](const ThreatMove &lhs, const ThreatMove &rhs)
		{
			float lhs_value = static_cast<int>(lhs.threat) + 0.1f * policy.at(lhs.move.row, lhs.move.col);
			float rhs_value = static_cast<int>(rhs.threat) + 0.1f * policy.at(rhs.move.row, rhs.move.col);
			return lhs_value > rhs_value;
		};
		std::sort(cross_threats.begin(), cross_threats.end(), compare);
		std::sort(circle_threats.begin(), circle_threats.end(), compare);

		std::vector<ThreatMove> &own_threats = (signToMove == Sign::CROSS) ? cross_threats : circle_threats;
		std::vector<ThreatMove> &opponent_threats = (signToMove == Sign::CROSS) ? circle_threats : cross_threats;

//		std::cout << "\nown threats\n";
//		for (size_t i = 0; i < own_threats.size(); i++)
//			std::cout << toString(own_threats[i].threat) << " at " << own_threats[i].move.toString() << " "
//					<< policy.at(own_threats[i].move.row, own_threats[i].move.col) << '\n';
//		std::cout << "opp threats\n";
//		for (size_t i = 0; i < opponent_threats.size(); i++)
//			std::cout << toString(opponent_threats[i].threat) << " at " << opponent_threats[i].move.toString() << " "
//					<< policy.at(opponent_threats[i].move.row, opponent_threats[i].move.col) << '\n';

		if (own_threats.size() > 0 and own_threats.front().threat == ThreatType::FIVE) // can make a five
		{
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::FIVE)
					moveList.push_back( { iter->move.toShort(), 1.0f });
			return ProvenValue::LOSS; // it is instant win, returning inverted value
		}
		if (opponent_threats.size() > 0 and opponent_threats.front().threat == ThreatType::FIVE) // opponent can make a five
		{
			for (auto iter = opponent_threats.begin(); iter < opponent_threats.end(); iter++)
				if (iter->threat == ThreatType::FIVE)
					moveList.push_back( { iter->move.toShort(), 1.0f });
			return ProvenValue::UNKNOWN;
		}
		if (own_threats.size() > 0 and own_threats.front().threat == ThreatType::OPEN_FOUR) // can make an open four
		{
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), policy.at(iter->move.row, iter->move.col) });
			return ProvenValue::LOSS; // it win in 3 plys, returning inverted value;
		}
		if (opponent_threats.size() > 0 and opponent_threats.front().threat == ThreatType::OPEN_FOUR) // opponent can make an open four
		{
			for (auto iter = opponent_threats.begin(); iter < opponent_threats.end(); iter++)
			{
				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), policy.at(iter->move.row, iter->move.col) });
				if (iter->threat == ThreatType::OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), policy.at(iter->move.row, iter->move.col) });
			}
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), policy.at(iter->move.row, iter->move.col) });
		}
//		if (own_threats.size() > 0)
//			return solve(signToMove);
//		else
			return ProvenValue::UNKNOWN;
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

	// private
	uint32_t FeatureExtractor::get_feature_at(int row, int col, Direction dir)
	{
		return features.at(pad + row, (pad + col) * 4 + static_cast<int>(dir));
	}
	Threat FeatureExtractor::get_threat_at(int row, int col, Direction dir)
	{
		return map_of_threats.at(pad + row, (pad + col) * 4 + static_cast<int>(dir));
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
	void FeatureExtractor::get_threat_lists(Sign signToMove)
	{
		cross_threats.clear();
		circle_threats.clear();

		const FeatureTable &table = get_feature_table(rules);
		for (int row = 0; row < internal_board.rows() - 2 * pad; row++)
			for (int col = 0; col < internal_board.cols() - 2 * pad; col++)
				if (internal_board.at(pad + row, pad + col) == 0)
				{
					Threat horizontal = table.getThreat(get_feature_at(row, col, Direction::HORIZONTAL));
					Threat vertical = table.getThreat(get_feature_at(row, col, Direction::VERTICAL));
					Threat diagonal = table.getThreat(get_feature_at(row, col, Direction::DIAGONAL));
					Threat antidiagonal = table.getThreat(get_feature_at(row, col, Direction::ANTIDIAGONAL));

					map_of_threats.at(pad + row, (pad + col) * 4 + 0) = horizontal;
					map_of_threats.at(pad + row, (pad + col) * 4 + 1) = vertical;
					map_of_threats.at(pad + row, (pad + col) * 4 + 2) = diagonal;
					map_of_threats.at(pad + row, (pad + col) * 4 + 3) = antidiagonal;

					ThreatType best_cross = std::max(std::max(horizontal.for_cross, vertical.for_cross),
							std::max(diagonal.for_cross, antidiagonal.for_cross));
					ThreatType best_circle = std::max(std::max(horizontal.for_circle, vertical.for_circle),
							std::max(diagonal.for_circle, antidiagonal.for_circle));

					// all moves are assigned signToMove so they can be directly used for move generation
					if (best_cross != ThreatType::NONE)
						cross_threats.push_back(ThreatMove(best_cross, Move(row, col, signToMove)));

					if (best_circle != ThreatType::NONE)
						circle_threats.push_back(ThreatMove(best_circle, Move(row, col, signToMove)));
				}
	}
	ProvenValue FeatureExtractor::solve(Sign signToMove)
	{
		std::vector<ThreatMove> &own_threats = (signToMove == Sign::CROSS) ? cross_threats : circle_threats;
		std::vector<ThreatMove> &opponent_threats = (signToMove == Sign::CROSS) ? circle_threats : cross_threats;
		std::reverse(own_threats.begin(), own_threats.end());
		opponent_threats.clear();

		std::cout << "\n\n\n\n\n\n";
		int depth = 0;
		while (own_threats.size() > 0)
		{
			std::cout << "\ndepth = " << depth << '\n';
			ThreatMove tm = own_threats.back();
			std::cout << tm.move.toString() << '\n';

			// make own move
			internal_board.at(pad + tm.move.row, pad + tm.move.col) = static_cast<int>(signToMove);
			calc_all_features();
			get_threat_lists(Sign::NONE);
			depth++;
			print();

			std::cout << "\nown threats\n";
			for (size_t i = 0; i < own_threats.size(); i++)
				std::cout << toString(own_threats[i].threat) << " at " << own_threats[i].move.toString() << '\n';
			std::cout << "opp threats\n";
			for (size_t i = 0; i < opponent_threats.size(); i++)
				std::cout << toString(opponent_threats[i].threat) << " at " << opponent_threats[i].move.toString() << '\n';

			bool opponent_can_have_five = std::any_of(opponent_threats.begin(), opponent_threats.end(), [](const ThreatMove &t)
			{	return t.threat == ThreatType::FIVE;});
			std::cout << "opponent can make five? = " << opponent_can_have_five << '\n';

			// make opponent response
			for (size_t i = 0; i < own_threats.size(); i++)
				if (own_threats[i].threat == ThreatType::FIVE)
				{
					internal_board.at(pad + own_threats[i].move.row, pad + own_threats[i].move.col) = static_cast<int>(invertSign(signToMove));
					calc_all_features();
					get_threat_lists(Sign::NONE);
					depth++;
					print();
					break;
				}
			std::cout << "\nown threats\n";
			for (size_t i = 0; i < own_threats.size(); i++)
				std::cout << toString(own_threats[i].threat) << " at " << own_threats[i].move.toString() << '\n';
			std::cout << "opp threats\n";
			for (size_t i = 0; i < opponent_threats.size(); i++)
				std::cout << toString(opponent_threats[i].threat) << " at " << opponent_threats[i].move.toString() << '\n';
		}

//		print();
//		printFeature(17, 7);
//		printFeature(17, 4);
//		printFeature(17, 6);
//		printFeature(15, 7);

//		add_move<4>(features, internal_board, Move(16, 5, Sign::CIRCLE));
//		printFeature(17, 6);
//		print();
//		delete_move<4>(features, internal_board, Move(16, 5, Sign::CIRCLE));
//		printFeature(17, 6);
//		print();

		std::cout << "\n\n\n\n\n\n";
		return ProvenValue::UNKNOWN;
	}
	void FeatureExtractor::update_threats(int row, int col)
	{
		const FeatureTable &table = get_feature_table(rules);

		for (int i = 0; i <= pad; i++)
		{
			int c = col + i;
			if (internal_board.at(row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::DIAGONAL));
				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::DIAGONAL)) = t;
			}
			c = pad + col;
			if (internal_board.at(row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::VERTICAL));
				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::VERTICAL)) = t;
			}
			c = 2 * pad + col - i;
			if (internal_board.at(row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(row + i, c, Direction::ANTIDIAGONAL));
				map_of_threats.at(row + i, c * 4 + static_cast<int>(Direction::ANTIDIAGONAL)) = t;
			}
		}

		for (int i = pad + col; i <= col + 2 * pad; i++)
			if (internal_board.at(row + i, col) == 0)
			{
				Threat t = table.getThreat(get_feature_at(row + i, col, Direction::HORIZONTAL));
				map_of_threats.at(row + i, col * 4 + static_cast<int>(Direction::HORIZONTAL)) = t;
			}

		for (int i = 1; i <= pad; i++)
		{
			int c = pad + col - i;
			if (internal_board.at(pad + row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::ANTIDIAGONAL));
				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::ANTIDIAGONAL)) = t;
			}
			c = pad + col;
			if (internal_board.at(pad + row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::VERTICAL));
				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::VERTICAL)) = t;
			}
			c = pad + col + i;
			if (internal_board.at(pad + row + i, c) == 0)
			{
				Threat t = table.getThreat(get_feature_at(pad + row + i, c, Direction::DIAGONAL));
				map_of_threats.at(pad + row + i, c * 4 + static_cast<int>(Direction::DIAGONAL)) = t;
			}
		}
	}

} /* namespace ag */

