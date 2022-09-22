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

#include <cinttypes>
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
			uint64_t policy_prior :20;
			uint64_t win_rate :22;
			uint64_t draw_rate :22;
			uint32_t visits = 0;
			uint16_t move = 0;
			uint16_t virtual_loss :14;
			uint16_t proven_value :2;
		public:

			Edge() noexcept :
					policy_prior(0),
					win_rate(0),
					draw_rate(0),
					virtual_loss(0),
					proven_value(static_cast<uint64_t>(ProvenValue::UNKNOWN))
			{
			}
			Edge(Move m) noexcept :
					policy_prior(0),
					win_rate(0),
					draw_rate(0),
					move(m.toShort()),
					virtual_loss(0),
					proven_value(static_cast<uint64_t>(ProvenValue::UNKNOWN))
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
				return policy_prior / 1048575.0f;
			}
			float getWinRate() const noexcept
			{
				return win_rate / 4194303.0f;
			}
			float getDrawRate() const noexcept
			{
				return draw_rate / 4194303.0f;
			}
			float getLossRate() const noexcept
			{
				return 1.0f - (win_rate + draw_rate) / 4194303.0f;
			}
			Value getValue() const noexcept
			{
				return Value(getWinRate(), getDrawRate(), getLossRate());
			}
			int getVisits() const noexcept
			{
				return visits;
			}
			Move getMove() const noexcept
			{
				return Move::move_from_short(move);
			}
			ProvenValue getProvenValue() const noexcept
			{
				return static_cast<ProvenValue>(proven_value);
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
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
				policy_prior = static_cast<uint64_t>(p * 1048575.0f);
			}
			void setValue(Value value) noexcept
			{
				win_rate = static_cast<uint64_t>(value.win * 4194303.0f);
				draw_rate = static_cast<uint64_t>(value.draw * 4194303.0f);
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				float tmp_win_rate = getWinRate();
				float tmp_draw_rate = getDrawRate();
				tmp_win_rate += (eval.win - tmp_win_rate) * tmp;
				tmp_draw_rate += (eval.draw - tmp_draw_rate) * tmp;
				win_rate = static_cast<uint64_t>(tmp_win_rate * 4194303.0f);
				draw_rate = static_cast<uint64_t>(tmp_draw_rate * 4194303.0f);
			}
			void setMove(Move m) noexcept
			{
				move = m.toShort();
			}
			void setProvenValue(ProvenValue pv) noexcept
			{
				proven_value = static_cast<uint64_t>(pv);
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
