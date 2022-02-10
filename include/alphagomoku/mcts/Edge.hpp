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
			int32_t visits = 0;
			Move move;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			int16_t virtual_loss = 0;
		public:
			Edge() noexcept = default;
			Edge(Move m) noexcept :
					move(m)
			{
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
			float getLossRate() const noexcept
			{
				return 1.0f - win_rate - draw_rate;
			}
			Value getValue() const noexcept
			{
				return Value(win_rate, draw_rate, getLossRate());
			}
			int getVisits() const noexcept
			{
				return visits;
			}
			Move getMove() const noexcept
			{
				return move;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
			}
			bool isProven() const noexcept
			{
				return proven_value != ProvenValue::UNKNOWN;
			}
			int getVirtualLoss() const noexcept
			{
				return virtual_loss;
			}

			void setNode(Node *ptr) noexcept
			{
				assert(ptr != nullptr);
				node = ptr;
			}
			void setPolicyPrior(float p) noexcept
			{
				policy_prior = p;
			}
			void setValue(Value value) noexcept
			{
				win_rate = value.win;
				draw_rate = value.draw;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				win_rate += (eval.win - win_rate) * tmp;
				draw_rate += (eval.draw - draw_rate) * tmp;
			}
			void setMove(Move m) noexcept
			{
				move = m;
			}
			void setProvenValue(ProvenValue pv) noexcept
			{
				proven_value = pv;
			}
			void increaseVirtualLoss() noexcept
			{
				assert(virtual_loss < std::numeric_limits<int16_t>::max());
				virtual_loss++;
			}
			void decreaseVirtualLoss() noexcept
			{
				assert(virtual_loss > 0);
				virtual_loss--;
			}
			void clearVirtualLoss() noexcept
			{
				virtual_loss = 0;
			}
			std::string toString() const;
			Edge copyInfo() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_HPP_ */
