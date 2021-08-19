/*
 * Node.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_NODE_HPP_
#define ALPHAGOMOKU_MCTS_NODE_HPP_

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
			Node *children = nullptr;
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
			const Node& getChild(int index) const noexcept
			{
				assert(index >= 0 && index < numberOfChildren());
				return children[index];
			}
			Node& getChild(int index) noexcept
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
			void createChildren(Node *ptr, int number) noexcept
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

			Node* begin() noexcept
			{
				return children;
			}
			Node* end() noexcept
			{
				return children + numberOfChildren();
			}
			const Node* begin() const noexcept
			{
				return children;
			}
			const Node* end() const noexcept
			{
				return children + numberOfChildren();
			}
	};

	struct MaxExpectation
	{
			float operator()(const Node *n) const noexcept
			{
				return n->getValue().win + 0.5f * n->getValue().draw;
			}
	};
	struct MaxWin
	{
			float operator()(const Node *n) const noexcept
			{
				return n->getValue().win;
			}
	};
	struct MaxNonLoss
	{
			float operator()(const Node *n) const noexcept
			{
				return n->getValue().win + n->getValue().draw;
			}
	};
	struct MaxBalance
	{
			float operator()(const Node *n) const noexcept
			{
				return -fabsf(n->getValue().win - n->getValue().loss);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODE_HPP_ */
