/*
 * SearchData.cpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>

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
		offset += sizeof(Value);

		minimax_score = Score::from_short(so.load<uint16_t>(offset));
		offset += sizeof(Score);

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
			actions[i].sign_and_visit_count = (actions[i].sign_and_visit_count & 0xFFFC) | static_cast<uint16_t>(board[i]);
	}
	void SearchData::setSearchResults(const Node &node) noexcept
	{
		for (Edge *edge = node.begin(); edge < node.end(); edge++)
		{
			const Move m = edge->getMove();

			assert(edge->getVisits() < 4096);
			const uint16_t tmp = (actions.at(m.row, m.col).sign_and_visit_count & 0x0003u) | (static_cast<uint16_t>(edge->getVisits()) << 2u);
			actions.at(m.row, m.col).sign_and_visit_count = tmp;

			actions.at(m.row, m.col).policy_prior = static_cast<int>(edge->getPolicyPrior() * scale);
			actions.at(m.row, m.col).score = Score::to_short(edge->getScore());
			actions.at(m.row, m.col).win_rate = static_cast<int>(edge->getValue().win_rate * scale);
			actions.at(m.row, m.col).draw_rate = static_cast<int>(edge->getValue().draw_rate * scale);
		}
		minimax_value = node.getValue();
		minimax_score = node.getScore();
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
		return static_cast<Sign>(actions.at(row, col).sign_and_visit_count & 0x0003);
	}
	int SearchData::getVisitCount(int row, int col) const noexcept
	{
		return (actions.at(row, col).sign_and_visit_count >> 2u);
	}
	float SearchData::getPolicyPrior(int row, int col) const noexcept
	{
		return actions.at(row, col).policy_prior / scale;
	}
	Score SearchData::getActionScore(int row, int col) const noexcept
	{
		return actions.at(row, col).score;
	}
	Value SearchData::getActionValue(int row, int col) const noexcept
	{
		return Value(actions.at(row, col).win_rate / scale, actions.at(row, col).draw_rate / scale);
	}

	Score SearchData::getMinimaxScore() const noexcept
	{
		return minimax_score;
	}
	Value SearchData::getMinimaxValue() const noexcept
	{
		return minimax_value;
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
		binary_data.save<uint16_t>(Score::to_short(minimax_score));
		binary_data.save<Value>(minimax_value);
		binary_data.save<int>(static_cast<int>(game_outcome));
		binary_data.save<Move>(played_move);
	}
	void SearchData::print() const
	{
		matrix<Sign> board(rows(), cols());
		matrix<Score> action_scores(rows(), cols());
		matrix<float> policy_prior(rows(), cols());
		matrix<Value> action_values(rows(), cols());

		for (int r = 0; r < rows(); r++)
			for (int c = 0; c < cols(); c++)
			{
				board.at(r, c) = getSign(r, c);
				action_scores.at(r, c) = getActionScore(r, c);
				policy_prior.at(r, c) = getPolicyPrior(r, c);
				action_values.at(r, c) = getActionValue(r, c);
			}

		std::cout << "next move " << played_move.toString() << '\n';
		std::cout << "minimax " << minimax_value.toString() << ", proven " << minimax_score.toString() << '\n';
		std::cout << "game outcome " << toString(game_outcome) << '\n';
		std::cout << "board\n" << Board::toString(board);
		std::cout << "policy\n" << Board::toString(board, policy_prior);
		std::cout << "proven values\n" << Board::toString(board, action_scores);
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

