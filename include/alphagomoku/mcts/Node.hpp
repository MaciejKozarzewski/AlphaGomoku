/*
 * Node_old.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_Node_old_HPP_
#define ALPHAGOMOKU_MCTS_Node_old_HPP_

#include <alphagomoku/mcts/Edge.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <string>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <cstring>

namespace ag
{
	class Node
	{
		private:
			Edge *edges = nullptr; // non-owning
			float win_rate = 0.0f;
			float draw_rate = 0.0f;
			uint32_t visits = 0;
			/* following fields need to be packed inside 4 bytes */
			ProvenValue proven_value :2;
			bool is_transposition :1;
			uint32_t number_of_edges :10;
			uint32_t depth :10;
			uint32_t virtual_loss :9; // effectively limits maximum batch size to 512
		public:
			void clear() noexcept
			{
				std::memset(this, 0, sizeof(Node));
			}
			bool isLeaf() const noexcept
			{
				return edges == nullptr;
			}
			Edge& getEdge(uint32_t index) const noexcept
			{
				assert(index < numberOfEdges());
				assert(edges != nullptr);
				return edges[index];
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
			uint32_t getVisits() const noexcept
			{
				return visits;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
			}
			bool isTransposition() const noexcept
			{
				return is_transposition;
			}
			uint32_t numberOfEdges() const noexcept
			{
				return number_of_edges;
			}
			uint32_t getDepth() const noexcept
			{
				return depth;
			}
			uint32_t getVirtualLoss() const noexcept
			{
				return virtual_loss;
			}
			void assignEdges(Edge *ptr, uint32_t number) noexcept
			{
				assert(number < 1024u);
				assert(ptr != nullptr);
				edges = ptr;
				number_of_edges = number;
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
			void markAsTransposition() noexcept
			{
				assert(isTransposition() == false);
				is_transposition = true;
			}
			void setDepth(uint32_t d) noexcept
			{
				depth = d;
			}
			void applyVirtualLoss() noexcept
			{
				assert(virtual_loss < 512u);
				virtual_loss++;
			}
			void cancelVirtualLoss() noexcept
			{
				assert(virtual_loss > 0u);
				virtual_loss--;
			}
			Edge* begin() const noexcept
			{
				assert(edges != nullptr);
				return edges;
			}
			Edge* end() const noexcept
			{
				assert(edges != nullptr);
				return edges + numberOfEdges();
			}
			std::string toString() const;
			void sortEdges() const;
	};

	class Node_old
	{
		private:
			Node_old *children = nullptr;
			float policy_prior = 0.0f;
			Value value;

			int32_t visits = 0;
			uint16_t move = 0;
			uint16_t data = 0; // 0:2 proven value, 2:16 number of children
		public:
			void clear() noexcept
			{
				children = nullptr;
				policy_prior = 0.0f;
				value = { 0.0f, 0.0f, 0.0f };
				visits = 0;
				move = data = 0;
			}
			const Node_old& getChild(int index) const noexcept
			{
				assert(index >= 0 && index < numberOfChildren());
				return children[index];
			}
			Node_old& getChild(int index) noexcept
			{
				assert(index >= 0 && index < numberOfChildren());
				return children[index];
			}
			float getPolicyPrior() const noexcept
			{
				return policy_prior;
			}
			Value getValue() const noexcept
			{
				return value;
			}
			int32_t getVisits() const noexcept
			{
				return visits;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return static_cast<ProvenValue>(data & 3);
			}
			uint16_t getMove() const noexcept
			{
				return move;
			}
			int numberOfChildren() const noexcept
			{
				return static_cast<int>(data >> 2);
			}
			void createChildren(Node_old *ptr, int number) noexcept
			{
				assert(number >= 0 && number < 16384);
				assert(ptr != nullptr);
				children = ptr;
				data = (data & 3) | (static_cast<uint16_t>(number) << 2);
			}
			void setPolicyPrior(float p) noexcept
			{
				assert(p >= 0.0f && p <= 1.0f);
				policy_prior = p;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				value.win += (eval.win - value.win) * tmp;
				value.draw += (eval.draw - value.draw) * tmp;
				value.loss += (eval.loss - value.loss) * tmp;
			}
			void applyVirtualLoss() noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				value.win -= value.win * tmp;
				value.draw -= value.draw * tmp;
				value.loss += (1.0f - value.loss) * tmp;
			}
			void cancelVirtualLoss() noexcept
			{
				assert(visits > 1);
				visits--;
				const float tmp = 1.0f / static_cast<float>(visits);
				value.win += value.win * tmp;
				value.draw += value.draw * tmp;
				value.loss -= (1.0f - value.loss) * tmp;
			}
			void setProvenValue(ProvenValue ev) noexcept
			{
				data = (data & 16383) | static_cast<uint16_t>(ev);
			}
			void setMove(uint16_t m) noexcept
			{
				move = m;
			}
			void setMove(const Move &m) noexcept
			{
				move = m.toShort();
			}
			bool isLeaf() const noexcept
			{
				return children == nullptr;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
			}
			std::string toString() const;
			void sortChildren() const;

			Node_old* begin() noexcept
			{
				return children;
			}
			Node_old* end() noexcept
			{
				return children + numberOfChildren();
			}
			const Node_old* begin() const noexcept
			{
				return children;
			}
			const Node_old* end() const noexcept
			{
				return children + numberOfChildren();
			}
	};

	struct MaxExpectation
	{
			float operator()(const Node_old *n) const noexcept
			{
				return n->getValue().win + 0.5f * n->getValue().draw;
			}
	};
	struct MaxWin
	{
			float operator()(const Node_old *n) const noexcept
			{
				return n->getValue().win;
			}
	};
	struct MaxNonLoss
	{
			float operator()(const Node_old *n) const noexcept
			{
				return n->getValue().win + n->getValue().draw;
			}
	};
	struct MaxBalance
	{
			float operator()(const Node_old *n) const noexcept
			{
				return -fabsf(n->getValue().win - n->getValue().loss);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_Node_old_HPP_ */
