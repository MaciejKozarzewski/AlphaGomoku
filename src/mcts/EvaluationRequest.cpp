/*
 * EvaluationRequest.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace ag
{
	EvaluationRequest::EvaluationRequest(int rows, int cols) :
			board(rows, cols),
			policy(rows, cols)
	{
	}
	void EvaluationRequest::clear() noexcept
	{
		board.clear();
		policy.clear();
		node = nullptr;
		augment_mode = -100;
		last_move = Move();
		value = 0.0f;
		proven_value = ProvenValue::UNKNOWN;
		is_ready = false;

	}
	void EvaluationRequest::setTrajectory(const matrix<Sign> &base_board, const SearchTrajectory &trajectory) noexcept
	{
		augment_mode = -100;
		node = &(trajectory.getLeafNode());
		last_move = trajectory.getLastMove();
		value = 0.0f;
		proven_value = ProvenValue::UNKNOWN;
		is_ready = false;
		board.copyFrom(base_board);
		policy.clear();
		for (int i = 1; i < trajectory.length(); i++)
		{
			Move move = trajectory.getMove(i);
			board.at(move.row, move.col) = move.sign;
		}
	}

	std::string EvaluationRequest::toString() const
	{
		std::string result;
		result += "sign = " + getSignToMove() + '\n';
		if (is_ready)
		{
			result += "value = " + std::to_string(value);
			if (proven_value != ProvenValue::UNKNOWN)
				result += " : proven " + ag::toString(proven_value);
			result += '\n';
			result += policyToString(board, policy);
		}
		else
			result += boardToString(board);
		return result;
	}
	void EvaluationRequest::augment() noexcept
	{
		if (augment_mode == -100)
		{
			augment_mode = randInt(4 + 4 * static_cast<int>(board.isSquare()));
			ag::augment(board, augment_mode);
		}
		else
		{
			ag::augment(board, -augment_mode);
			ag::augment(policy, -augment_mode);
			augment_mode = -100;
		}
	}
}

