/*
 * Node.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_NODE_HPP_
#define ALPHAGOMOKU_MCTS_NODE_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <string>
#include <assert.h>
#include <inttypes.h>
#include <cstring>

namespace ag
{
	enum ProvenValue
	{
		UNKNOWN, WIN, LOSS, DRAW
	};

	std::string toString(ProvenValue pv);

	class Node
	{
		private:
			Node *children = nullptr;
			float policy_prior = 0.0f;
			float value = 0.0f;
			int32_t visits = 0;
			uint16_t move = 0;
			uint16_t data = 0; // 0:2 exact value, 2:16 number of children
		public:
			void clear() noexcept
			{
				std::memset(this, 0, sizeof(Node));
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
			float getValue() const noexcept
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
			void updateValue(float eval) noexcept
			{
				assert(eval >= 0.0f && eval <= 1.0f);
				visits++;
				value += (eval - value) / static_cast<float>(visits);
			}
			void applyVirtualLoss() noexcept
			{
				visits++;
				value -= value / static_cast<float>(visits);
			}
			void cancelVirtualLoss() noexcept
			{
				assert(visits > 1);
				visits--;
				value *= (1.0f + 1.0f / static_cast<float>(visits));
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
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODE_HPP_ */
