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
	void EvaluationRequest::setTrajectory(const matrix<Sign> &base_board, const SearchTrajectory &trajectory) noexcept
	{
		augment_mode = -100;
		node = &(trajectory.getLeafNode());
		sign_to_move = invertSign(trajectory.getLastMove().sign);
		value = 0.0f;
		is_ready = false;
		board.copyFrom(base_board);
		policy.clear();
		for (int i = 1; i < trajectory.length(); i++)
		{
			Move move = trajectory.getMove(i);
			board.at(move.row, move.col) = move.sign;
		}
	}

	void EvaluationRequest::setPolicy(const float *p) noexcept
	{
		std::memcpy(policy.data(), p, policy.sizeInBytes());
	}
	void EvaluationRequest::setValue(float v) noexcept
	{
		value = v;
	}
	std::string EvaluationRequest::toString() const
	{
		std::string result;
		result += "sign = " + sign_to_move + '\n';
		if (is_ready)
		{
			result += "value = " + std::to_string(value) + '\n';
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

