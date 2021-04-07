/*
 * EvaluationRequest.hpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_
#define ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <string>

namespace ag
{
	class SearchTrajectory;
} /* namespace ag */

namespace ag
{
	class EvaluationRequest
	{
		public:
			matrix<Sign> board;
			matrix<float> policy;
			Node *node = nullptr;
			Move last_move;
			Value value;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			bool is_ready = false;

		public:
			EvaluationRequest(int rows, int cols);

			void clear() noexcept;
			void setTrajectory(const matrix<Sign> &base_board, const SearchTrajectory &trajectory) noexcept;
			Node* getNode() const noexcept
			{
				return node;
			}
			Move getLastMove() const noexcept
			{
				return last_move;
			}
			Sign getSignToMove() const noexcept
			{
				return invertSign(last_move.sign);
			}
			Value getValue() const noexcept
			{
				return value;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
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
			void setBoard(const matrix<Sign> &other) noexcept
			{
				this->board.copyFrom(other);
			}
			void setPolicy(const float *p) noexcept
			{
				std::memcpy(policy.data(), p, policy.sizeInBytes());
			}
			void setValue(Value v) noexcept
			{
				value = v;
			}
			void setProvenValue(ProvenValue pv) noexcept
			{
				proven_value = pv;
			}
			void setLastMove(Move move) noexcept
			{
				last_move = move;
			}
			void setNode(Node *n) noexcept
			{
				node = n;
			}
			std::string toString() const;
	};
}

#endif /* ALPHAGOMOKU_SEARCH_EVALUATIONREQUEST_HPP_ */
