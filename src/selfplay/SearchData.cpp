/*
 * SearchData.cpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <minml/utils/serialization.hpp>

#include <iostream>

namespace ag
{

	SearchData::SearchData(const SerializedObject &so, size_t &offset)
	{
		const int rows = so.load<int>(offset);
		offset += sizeof(rows);
		const int cols = so.load<int>(offset);
		offset += sizeof(cols);

		actions = matrix<internal_data>(rows, cols);
		so.load(actions.data(), offset, actions.sizeInBytes());
		offset += actions.sizeInBytes();

		minimax_value = so.load<Value>(offset);
		offset += sizeof(minimax_value);

		proven_value = static_cast<ProvenValue>(so.load<int>(offset));
		offset += sizeof(int);

		game_outcome = static_cast<GameOutcome>(so.load<int>(offset));
		offset += sizeof(int);

		played_move = so.load<Move>(offset);
		offset += sizeof(Move);
	}
	SearchData::SearchData(int rows, int cols) :
			actions(rows, cols)
	{
	}

	int SearchData::rows() const noexcept
	{
		return actions.rows();
	}
	int SearchData::cols() const noexcept
	{
		return actions.cols();
	}
	void SearchData::setBoard(const matrix<Sign> &board) noexcept
	{
		for (int i = 0; i < board.size(); i++)
			actions[i].sign = static_cast<int>(board[i]);
	}
	void SearchData::setSearchResults(const Node &node) noexcept
	{
		for (Edge *edge = node.begin(); edge < node.end(); edge++)
		{
			assert(edge->getVisits() < 4096);
			const Move m = edge->getMove();
			actions.at(m.row, m.col).proven_value = static_cast<int>(edge->getProvenValue());
			actions.at(m.row, m.col).visit_count = static_cast<int>(edge->getVisits());
			actions.at(m.row, m.col).policy_prior = static_cast<int>(edge->getPolicyPrior() * p_scale);
			actions.at(m.row, m.col).win = static_cast<int>(edge->getValue().win * q_scale);
			actions.at(m.row, m.col).draw = static_cast<int>(edge->getValue().draw * q_scale);
		}
		minimax_value = node.getValue();
		proven_value = node.getProvenValue();
	}
	void SearchData::setOutcome(GameOutcome gameOutcome) noexcept
	{
		game_outcome = gameOutcome;
	}
	void SearchData::setMove(Move m) noexcept
	{
		played_move = m;
	}

	Sign SearchData::getSign(int row, int col) const noexcept
	{
		return static_cast<Sign>(actions.at(row, col).sign);
	}
	ProvenValue SearchData::getActionProvenValue(int row, int col) const noexcept
	{
		return static_cast<ProvenValue>(actions.at(row, col).proven_value);
	}
	int SearchData::getVisitCount(int row, int col) const noexcept
	{
		return actions.at(row, col).visit_count;
	}
	float SearchData::getPolicyPrior(int row, int col) const noexcept
	{
		constexpr float scale = 1.0f / p_scale;
		return actions.at(row, col).policy_prior * scale;
	}
	Value SearchData::getActionValue(int row, int col) const noexcept
	{
		constexpr float scale = 1.0f / q_scale;
		if (getVisitCount(row, col))
			return Value(actions.at(row, col).win * scale, actions.at(row, col).draw * scale);
		else
			return Value();
	}
	Value SearchData::getMinimaxValue() const noexcept
	{
		return minimax_value;
	}
	ProvenValue SearchData::getProvenValue() const noexcept
	{
		return proven_value;
	}
	GameOutcome SearchData::getOutcome() const noexcept
	{
		return game_outcome;
	}
	Move SearchData::getMove() const noexcept
	{
		return played_move;
	}
	void SearchData::serialize(SerializedObject &binary_data) const
	{
		binary_data.save<int>(actions.rows());
		binary_data.save<int>(actions.cols());
		binary_data.save(actions.data(), actions.sizeInBytes());
		binary_data.save<Value>(minimax_value);
		binary_data.save<int>(static_cast<int>(proven_value));
		binary_data.save<int>(static_cast<int>(game_outcome));
		binary_data.save<Move>(played_move);
	}
	void SearchData::print() const
	{
		matrix<Sign> board(rows(), cols());
		matrix<ProvenValue> proven_values(rows(), cols());
		matrix<float> policy_prior(rows(), cols());
		matrix<Value> action_values(rows(), cols());

		for (int r = 0; r < rows(); r++)
			for (int c = 0; c < cols(); c++)
			{
				board.at(r, c) = getSign(r, c);
				proven_values.at(r, c) = getActionProvenValue(r, c);
				policy_prior.at(r, c) = getPolicyPrior(r, c);
				action_values.at(r, c) = getActionValue(r, c);
			}

		std::cout << "next move " << played_move.toString() << '\n';
		std::cout << "minimax " << minimax_value.toString() << ", proven " << toString(proven_value) << '\n';
		std::cout << "game outcome " << toString(game_outcome) << '\n';
		std::cout << "board\n" << Board::toString(board);
		std::cout << "policy\n" << Board::toString(board, policy_prior);
		std::cout << "proven values\n" << Board::toString(board, proven_values);
		std::cout << "action values\n" << Board::toString(board, action_values);
	}
	bool SearchData::isCorrect() const noexcept
	{
		return true;
//		if (minimax_value < 0.0f || minimax_value > 1.0f)
//			return false;

		Move m(played_move);
		if (m.row < 0 || m.row >= actions.rows())
			return false;
		if (m.col < 0 || m.col >= actions.cols())
			return false;
		if (m.sign != Sign::CROSS && m.sign != Sign::CIRCLE)
			return false;

		matrix<Sign> board(actions.rows(), actions.cols());
		matrix<float> policy(actions.rows(), actions.cols());
//		Sign sign_to_move;
//		copyTo(board, policy, sign_to_move);

		int count_X = 0, count_O = 0, count_empty = 0;
		for (int i = 0; i < actions.rows(); i++)
			for (int j = 0; j < actions.cols(); j++)
			{
				if (board.at(i, j) < Sign::NONE || board.at(i, j) > Sign::CIRCLE)
					return false;
				if (board.at(i, j) == Sign::NONE)
					count_empty++;
				else
				{
					if (board.at(i, j) == Sign::CROSS)
						count_X++;
					if (board.at(i, j) == Sign::CIRCLE)
						count_O++;
				}
			}
		if ((count_O + count_X + count_empty) != actions.size())
			return false;
		if (abs(count_O - count_X) > 1)
		{
			std::cout << count_O << " " << count_X << " " << count_empty << '\n';
			return false;
		}

//		float policy_sum = 0.0f;
//		for (int i = 0; i < state.rows(); i++)
//			for (int j = 0; j < state.cols(); j++)
//			{
//				if (policy.at(i, j) < 0.0f || policy.at(i, j) > 1.0f)
//					return false;
//				policy_sum += policy.at(i, j);
//			}
//		if (abs(policy_sum - 1.0f) > 0.1f)
//			return false;

//		for (int i = 0; i < state.rows(); i++)
//			for (int j = 0; j < state.cols(); j++)
//				if (board.at(i, j) != Sign::NONE && policy.at(i, j) != 0.0)
//				{
//					std::cout << i << "," << j << " " << board.at(i, j) << " " << policy.at(i, j) << std::endl;
//					std::cout << policyToString(board, policy) << '\n';
//					board.clear();
//					std::cout << boardToString(board) << '\n';
//					return false;
//				}

		return true;
	}

} /* namespace ag */

