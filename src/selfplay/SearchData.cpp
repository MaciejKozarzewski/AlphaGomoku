/*
 * SearchData.cpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <libml/utils/serialization.hpp>

#include <iostream>

namespace ag
{

	SearchData::SearchData(const SerializedObject &so, size_t &offset)
	{
		int rows = so.load<int>(offset);
		offset += sizeof(rows);
		int cols = so.load<int>(offset);
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

//		played_move = so.load<Move>(offset);
//		offset += sizeof(Move);
		played_move = Move::move_from_short(so.load<uint16_t>(offset));
		offset += sizeof(uint16_t);
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
			actions.data()[i].sign = static_cast<int>(board.data()[i]);
	}
	void SearchData::setActionProvenValues(const matrix<ProvenValue> &provenValues) noexcept
	{
		for (int i = 0; i < provenValues.size(); i++)
			actions.data()[i].pv = static_cast<int>(provenValues.data()[i]);
	}
	void SearchData::setPolicy(const matrix<float> &policy) noexcept
	{
		const float scale = 262143.0f;
		for (int i = 0; i < policy.size(); i++)
			actions.data()[i].prior = static_cast<int>(policy.data()[i] * scale);
	}
	void SearchData::setActionValues(const matrix<Value> &actionValues) noexcept
	{
		const float scale = 16383.0f;
		for (int i = 0; i < actionValues.size(); i++)
		{
			actions.data()[i].win = static_cast<int>(actionValues.data()[i].win * scale);
			actions.data()[i].draw = static_cast<int>(actionValues.data()[i].draw * scale);
			actions.data()[i].loss = static_cast<int>(actionValues.data()[i].loss * scale);
		}
	}
	void SearchData::setMinimaxValue(Value minimaxValue) noexcept
	{
		minimax_value = minimaxValue;
	}
	void SearchData::setProvenValue(ProvenValue pv) noexcept
	{
		proven_value = pv;
	}
	void SearchData::setOutcome(GameOutcome gameOutcome) noexcept
	{
		game_outcome = gameOutcome;
	}
	void SearchData::setMove(Move m) noexcept
	{
		played_move = m;
	}

	void SearchData::getBoard(matrix<Sign> &board) const noexcept
	{
		for (int i = 0; i < board.size(); i++)
			board.data()[i] = static_cast<Sign>(actions.data()[i].sign);
	}
	void SearchData::getActionProvenValues(matrix<ProvenValue> &provenValues) const noexcept
	{
		for (int i = 0; i < provenValues.size(); i++)
			provenValues.data()[i] = static_cast<ProvenValue>(actions.data()[i].pv);
	}
	void SearchData::getPolicy(matrix<float> &policy) const noexcept
	{
		const float scale = 1.0f / 262143.0f;
		for (int i = 0; i < policy.size(); i++)
			policy.data()[i] = actions.data()[i].prior * scale;
	}
	void SearchData::getActionValues(matrix<Value> &actionValues) const noexcept
	{
		const float scale = 1.0f / 16383.0f;
		for (int i = 0; i < actionValues.size(); i++)
		{
			actionValues.data()[i].win = actions.data()[i].win * scale;
			actionValues.data()[i].draw = actions.data()[i].draw * scale;
			actionValues.data()[i].loss = actions.data()[i].loss * scale;
		}
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
//		binary_data.save<uint16_t>(played_move.toShort());
	}
	void SearchData::print() const
	{
		matrix<Sign> board(actions.rows(), actions.cols());
		getBoard(board);
		matrix<ProvenValue> proven_values(actions.rows(), actions.cols());
		getActionProvenValues(proven_values);
		matrix<float> policy(actions.rows(), actions.cols());
		getPolicy(policy);
		matrix<Value> action_values(actions.rows(), actions.cols());
		getActionValues(action_values);
		std::cout << "next move " << played_move.toString() << '\n';
		std::cout << "minimax " << minimax_value.toString() << ", proven " << toString(proven_value) << '\n';
		std::cout << "game outcome " << toString(game_outcome) << '\n';
		std::cout << "board\n" << boardToString(board);
		std::cout << "policy\n" << policyToString(board, policy);
		std::cout << "proven values\n" << provenValuesToString(board, proven_values);
		std::cout << "action values\n" << actionValuesToString(board, action_values);
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

