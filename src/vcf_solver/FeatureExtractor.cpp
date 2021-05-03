/*
 * FeatureExtractor.cpp
 *
 *  Created on: 2 maj 2021
 *      Author: maciek
 */

#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/utils/configs.hpp>

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
			features(internal_board.rows(), internal_board.cols() * 4)
	{
		internal_board.fill(3);
	}

	void FeatureExtractor::generateMoves(matrix<float> &policy, std::vector<std::pair<uint16_t, float>> &moveList, const matrix<Sign> &board,
			Sign signToMove)
	{
		static_assert(sizeof(Sign) == sizeof(int));
		assert(board.rows() + 2 * pad == internal_board.rows());
		assert(board.cols() + 2 * pad == internal_board.cols());

		for (int r = 0; r < board.rows(); r++)
			std::memcpy(internal_board.data(pad + r) + pad, board.data(r), sizeof(int) * board.cols());

		calc_all_features();
		get_threat_lists(board, signToMove);
		std::sort(cross_threats.begin(), cross_threats.end(), [](const ThreatMove &lhs, const ThreatMove &rhs)
		{	return lhs.threat > rhs.threat;});
		std::sort(circle_threats.begin(), circle_threats.end(), [](const ThreatMove &lhs, const ThreatMove &rhs)
		{	return lhs.threat > rhs.threat;});

		std::vector<ThreatMove> &own_threats = (signToMove == Sign::CROSS) ? cross_threats : circle_threats;
		std::vector<ThreatMove> &opponent_threats = (signToMove == Sign::CROSS) ? circle_threats : cross_threats;

//		std::cout << "\nown threats\n";
//		for (size_t i = 0; i < own_threats.size(); i++)
//			std::cout << toString(own_threats[i].threat) << " at " << own_threats[i].move.toString() << '\n';
//		std::cout << "opp threats\n";
//		for (size_t i = 0; i < opponent_threats.size(); i++)
//			std::cout << toString(opponent_threats[i].threat) << " at " << opponent_threats[i].move.toString() << '\n';

		if (own_threats.size() > 0 and own_threats.front().threat == ThreatType::FIVE) // can make a five
		{
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::FIVE)
					moveList.push_back( { iter->move.toShort(), 1.0f });
			return;
		}
		if (opponent_threats.size() > 0 and opponent_threats.front().threat == ThreatType::FIVE) // opponent can make a five
		{
			for (auto iter = opponent_threats.begin(); iter < opponent_threats.end(); iter++)
				if (iter->threat == ThreatType::FIVE)
					moveList.push_back( { iter->move.toShort(), 1.0f });
			return;
		}
		if (own_threats.size() > 0 and own_threats.front().threat == ThreatType::OPEN_FOUR) // can make an open four
		{
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), policy.at(iter->move.row, iter->move.col) });
			return;
		}
		if (opponent_threats.size() > 0 and opponent_threats.front().threat == ThreatType::OPEN_FOUR) // opponent can make an open four
		{
			for (auto iter = opponent_threats.begin(); iter < opponent_threats.end(); iter++)
			{
				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), std::max(0.0f, policy.at(iter->move.row, iter->move.col)) });
				if (iter->threat == ThreatType::OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), std::max(0.0f, policy.at(iter->move.row, iter->move.col)) });
			}
			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
					moveList.push_back( { iter->move.toShort(), std::max(0.02f, policy.at(iter->move.row, iter->move.col)) });
			return;
		}
//		if (own_threats.size() > 0 and own_threats.front().threat == ThreatType::HALF_OPEN_FOUR) // can make an open half open four
//		{
//			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
//				if (iter->threat == ThreatType::HALF_OPEN_FOUR) // slightly enhance prior on such moves
//					policy.at(iter->move.row, iter->move.col) = std::max(0.01f, policy.at(iter->move.row, iter->move.col));
//		}
//		if (opponent_threats.size() > 0 and opponent_threats.front().threat == ThreatType::HALF_OPEN_FOUR) // can make an open four
//		{
//			for (auto iter = opponent_threats.begin(); iter < opponent_threats.end(); iter++)
//				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
//					policy.at(iter->move.row, iter->move.col) = std::max(0.0f, policy.at(iter->move.row, iter->move.col));
//			for (auto iter = own_threats.begin(); iter < own_threats.end(); iter++)
//				if (iter->threat == ThreatType::HALF_OPEN_FOUR)
//					policy.at(iter->move.row, iter->move.col) = std::max(0.01f, policy.at(iter->move.row, iter->move.col));
//		}
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

			uint32_t line = get_feature_at(x, y, i);
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
	uint32_t FeatureExtractor::get_feature_at(int row, int col, int dir)
	{
		assert(dir >= 0 && dir < 4);
		return features.at(pad + row, (pad + col) * 4 + dir);
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
	void FeatureExtractor::get_threat_lists(const matrix<Sign> &board, Sign signToMove)
	{
		cross_threats.clear();
		circle_threats.clear();

		const FeatureTable &table = get_feature_table(rules);
		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
				if (board.at(row, col) == Sign::NONE)
				{
					Threat horizontal = table.getThreat(get_feature_at(row, col, 0));
					Threat vertical = table.getThreat(get_feature_at(row, col, 1));
					Threat diagonal = table.getThreat(get_feature_at(row, col, 2));
					Threat antidiagonal = table.getThreat(get_feature_at(row, col, 3));

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

} /* namespace ag */

