/*
 * Edge.hpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGE_HPP_
#define ALPHAGOMOKU_MCTS_EDGE_HPP_

#include <alphagomoku/mcts/Value.hpp>

#include <inttypes.h>
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
			Node *node = nullptr;
			float policy_prior = 0.0f;
			float win_rate = 0.0f;
			float draw_rate = 0.0f;

			int32_t visits = 0;
			uint16_t move = 0;
			uint16_t data = 0; // 0:2 proven value
		public:
			void clear() noexcept
			{
				node = nullptr;
				policy_prior = 0.0f;
				win_rate = 0.0f;
				draw_rate = 0.0f;
				visits = 0;
				move = 0;
				data = 0;
			}
			const Node* getNode() const noexcept
			{
				return node;
			}
			Node* getNode() noexcept
			{
				return node;
			}
			Value getValue() const noexcept
			{
				return Value(win_rate, draw_rate, 1.0f - win_rate - draw_rate);
			}
			int32_t getVisits() const noexcept
			{
				return visits;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return static_cast<ProvenValue>(data & 3);
			}
			void assignNode(Node *ptr) noexcept
			{
				assert(ptr != nullptr);
				node = ptr;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				win_rate += (eval.win - win_rate) * tmp;
				draw_rate += (eval.draw - draw_rate) * tmp;
			}
			void applyVirtualLoss() noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				win_rate -= win_rate * tmp;
				draw_rate -= draw_rate * tmp;
			}
			void cancelVirtualLoss() noexcept
			{
				assert(visits > 1);
				visits--;
				const float tmp = 1.0f / static_cast<float>(visits);
				win_rate += win_rate * tmp;
				draw_rate += draw_rate * tmp;
			}
			void setProvenValue(ProvenValue ev) noexcept
			{
				data = (data & 16383) | static_cast<uint16_t>(ev);
			}
			bool isLeaf() const noexcept
			{
				return node == nullptr;
			}
			bool isProven() const noexcept
			{
				return getProvenValue() != ProvenValue::UNKNOWN;
			}
			std::string toString() const;
			void sortChildren() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGE_HPP_ */
