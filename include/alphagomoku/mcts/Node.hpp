/*
 * Node_old.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_NODE_HPP_
#define ALPHAGOMOKU_MCTS_NODE_HPP_

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
		public:
			Node() = default;
			Node(const Node &other) :
					win_rate(other.win_rate),
					draw_rate(other.draw_rate),
					visits(other.visits),
					proven_value(other.proven_value),
					number_of_edges(0),
					depth(other.depth),
					sign_to_move(other.sign_to_move)
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
			}

			bool isLeaf() const noexcept
			{
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

			void createEdges(int number) noexcept
			{
				assert(number >= 0 && number < std::numeric_limits<int16_t>::max());
				if (number != number_of_edges)
				{
					edges = std::make_unique<Edge[]>(number);
					number_of_edges = static_cast<int16_t>(number);
				}
			}
			void setEdges(std::unique_ptr<Edge[]> &ptr, int number) noexcept
			{
				assert(number >= 0 && number < std::numeric_limits<int16_t>::max());
				edges = std::move(ptr);
				number_of_edges = static_cast<int16_t>(number);
			}
			std::unique_ptr<Edge[]> freeEdges() noexcept
			{
				number_of_edges = 0;
				return std::move(edges);
			}
			void setValue(Value value) noexcept
			{
				win_rate = value.win;
				draw_rate = value.draw;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
//				const float tmp = 1.0f / static_cast<float>(visits);
				const float tmp = std::max(1.0f / Edge::update_threshold, 1.0f / static_cast<float>(visits)); // TODO
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
			std::string toString() const;
			void sortEdges() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODE_HPP_ */
