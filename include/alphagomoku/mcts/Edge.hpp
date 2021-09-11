/*
 * Edge.hpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_HPP_

#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/game/Move.hpp>

#include <inttypes.h>
#include <cstring>
#include <cassert>

namespace ag
{
	class Node;
} /* namespace ag */

namespace ag
{
	class Edge
	{
		private:
			Node *node = nullptr; // non-owning
			float policy_prior = 0.0f;
			float win_rate = 0.0f;
			float draw_rate = 0.0f;
			uint32_t visits = 0;
			Move move;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			uint16_t virtual_loss = 0;
		public:
			void clear() noexcept
			{
				node = nullptr;
				policy_prior = win_rate = draw_rate = 0.0f;
				visits = 0;
				move = Move();
				proven_value = ProvenValue::UNKNOWN;
				virtual_loss = 0;
			}
			bool isLeaf() const noexcept
			{
				return node == nullptr;
			}
			Node* getNode() const noexcept
			{
				return node;
			}
			float getPolicyPrior() const noexcept
			{
				return policy_prior;
			}
			float getWinRate() const noexcept
			{
				return win_rate;
			}
			float getDrawRate() const noexcept
			{
				return draw_rate;
			}
			Value getValue() const noexcept
			{
				return Value(win_rate, draw_rate, 1.0f - win_rate - draw_rate);
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
			}
			uint32_t getVisits() const noexcept
			{
				return visits;
			}
			Move getMove() const noexcept
			{
				return move;
			}
			uint32_t getVirtualLoss() const noexcept
			{
				return virtual_loss;
			}
			void assignNode(Node *ptr) noexcept
			{
				assert(ptr != nullptr);
				node = ptr;
			}
			void setPolicyPrior(float p) noexcept
			{
				policy_prior = p;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				win_rate += (eval.win - win_rate) * tmp;
				draw_rate += (eval.draw - draw_rate) * tmp;
			}
			void setProvenValue(ProvenValue ev) noexcept
			{
				proven_value = ev;
			}
			void setMove(Move m) noexcept
			{
				move = m;
			}
			void applyVirtualLoss() noexcept
			{
				assert(virtual_loss < (1u << 16));
				virtual_loss++;
			}
			void cancelVirtualLoss() noexcept
			{
				assert(virtual_loss > 0u);
				virtual_loss--;
			}
			std::string toString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_HPP_ */
