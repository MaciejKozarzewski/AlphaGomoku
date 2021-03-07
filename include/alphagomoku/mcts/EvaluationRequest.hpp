/*
 * EvaluationRequest.hpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_
#define ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <string>

namespace ag
{
	class SearchTrajectory;
	class Node;
} /* namespace ag */

namespace ag
{
	class EvaluationRequest
	{
		public:
			int augment_mode = -100;
			Node *node = nullptr;
			Sign sign_to_move = Sign::NONE;
			float value = 0.0f;
			bool is_ready = false;
			matrix<Sign> board;
			matrix<float> policy;
		public:

			EvaluationRequest(int rows, int cols);

			void setTrajectory(const matrix<Sign> &base_board, const SearchTrajectory &trajectory) noexcept;
			Node* getNode() const noexcept
			{
				return node;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			float getValue() const noexcept
			{
				return value;
			}
			bool isReady() const noexcept
			{
				return is_ready;
			}
			const matrix<Sign>& getBoard() const noexcept
			{
				return board;
			}
			matrix<Sign>& getBoard() noexcept
			{
				return board;
			}
			const matrix<float>& getPolicy() const noexcept
			{
				return policy;
			}
			matrix<float>& getPolicy() noexcept
			{
				return policy;
			}

			void setReady() noexcept
			{
				is_ready = true;
			}
			void setPolicy(const float *p) noexcept;
			void setValue(float v) noexcept;
			void setSignToMove(Sign sign) noexcept
			{
				sign_to_move = sign;
			}
			std::string toString() const;
			void augment() noexcept;
	};
}

#endif /* ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_ */
