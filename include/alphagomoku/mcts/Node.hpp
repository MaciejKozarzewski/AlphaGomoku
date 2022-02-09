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
#include <memory>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <cstring>

namespace ag
{
	class Node
	{
		private:
			std::unique_ptr<Edge[]> edges;
			float win_rate = 0.0f;
			float draw_rate = 0.0f;
			int32_t visits = 0;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			int16_t number_of_edges = 0;
			int16_t depth = 0;
			Sign sign_to_move = Sign::NONE;
			bool is_used = false;
		public:
			Node() = default;
			Node(const Node &other) :
					win_rate(other.win_rate),
					draw_rate(other.draw_rate),
					visits(other.visits),
					proven_value(other.proven_value),
					number_of_edges(0),
					depth(other.depth),
					sign_to_move(other.sign_to_move),
					is_used(other.is_used)
			{
			}
			Node(Node &&other) = default;
			Node& operator=(const Node &other)
			{
				win_rate = other.win_rate;
				draw_rate = other.draw_rate;
				visits = other.visits;
				proven_value = other.proven_value;
				depth = other.depth;
				sign_to_move = other.sign_to_move;
				is_used = other.is_used;
				return *this;
			}
			Node& operator=(Node &&other) = default;

			void clear() noexcept
			{
				win_rate = draw_rate = 0.0f;
				visits = 0;
				proven_value = ProvenValue::UNKNOWN;
				depth = 0;
				sign_to_move = Sign::NONE;
				is_used = false;
			}

			bool isLeaf() const noexcept
			{
				assert(edges != nullptr && number_of_edges > 0);
				return edges == nullptr;
			}
			const Edge& getEdge(int index) const noexcept
			{
				assert(edges != nullptr);
				assert(index >= 0 && index < number_of_edges);
				return edges[index];
			}
			Edge& getEdge(int index) noexcept
			{
				assert(edges != nullptr);
				assert(index >= 0 && index < number_of_edges);
				return edges[index];
			}
			Edge* begin() const noexcept
			{
				assert(edges != nullptr);
				return edges.get();
			}
			Edge* end() const noexcept
			{
				assert(edges != nullptr);
				return edges.get() + number_of_edges;
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
			int getVisits() const noexcept
			{
				return visits;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
			}
			bool isProven() const noexcept
			{
				return proven_value != ProvenValue::UNKNOWN;
			}
			int numberOfEdges() const noexcept
			{
				return number_of_edges;
			}
			int getDepth() const noexcept
			{
				return depth;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			bool isUsed() const noexcept
			{
				return is_used;
			}

			void setEdges(int number) noexcept
			{
				assert(number >= 0 && number < std::numeric_limits<int16_t>::max());
				if (number != number_of_edges)
				{
					edges = std::make_unique<Edge[]>(number);
					number_of_edges = static_cast<int16_t>(number);
				}
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
			void setProvenValue(ProvenValue pv) noexcept
			{
				proven_value = pv;
			}
			void setDepth(int d) noexcept
			{
				assert(d >= 0 && d < std::numeric_limits<int16_t>::max());
				depth = d;
			}
			void setSignToMove(Sign s) noexcept
			{
				sign_to_move = s;
			}
			void markAsUsed() noexcept
			{
				assert(is_used == false);
				is_used = true;
			}
			void markAsUnused() noexcept
			{
				assert(is_used == true);
				is_used = false;
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
