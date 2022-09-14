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
#include <cassert>
#include <cinttypes>
#include <math.h>
#include <cstring>

namespace ag
{
	class Node
	{
		private:
			Edge *edges = nullptr;
			float win_rate = 0.0f;
			float draw_rate = 0.0f;
			int32_t visits = 0;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			int16_t number_of_edges = 0;
			int16_t depth = 0;
			Sign sign_to_move = Sign::NONE;
			bool is_owning = false;
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
			Node(Node &&other) noexcept :
					edges(other.edges),
					win_rate(other.win_rate),
					draw_rate(other.draw_rate),
					visits(other.visits),
					proven_value(other.proven_value),
					number_of_edges(other.number_of_edges),
					depth(other.depth),
					sign_to_move(other.sign_to_move),
					is_owning(other.is_owning)
			{
				other.edges = nullptr;
				other.number_of_edges = 0;
				other.is_owning = false;
			}
			Node& operator=(const Node &other)
			{
				edges = nullptr;
				win_rate = other.win_rate;
				draw_rate = other.draw_rate;
				visits = other.visits;
				proven_value = other.proven_value;
				number_of_edges = 0;
				depth = other.depth;
				sign_to_move = other.sign_to_move;
				is_owning = false;
				return *this;
			}
			Node& operator=(Node &&other) noexcept
			{
				std::swap(this->edges, other.edges);
				std::swap(this->win_rate, other.win_rate);
				std::swap(this->draw_rate, other.draw_rate);
				std::swap(this->visits, other.visits);
				std::swap(this->proven_value, other.proven_value);
				std::swap(this->number_of_edges, other.number_of_edges);
				std::swap(this->depth, other.depth);
				std::swap(this->sign_to_move, other.sign_to_move);
				std::swap(this->is_owning, other.is_owning);
				return *this;
			}
			~Node()
			{
				if (is_owning)
					delete[] edges;
			}

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
				return edges;
			}
			Edge* end() const noexcept
			{
				assert(edges != nullptr);
				return edges + number_of_edges;
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
				if (is_owning and number != number_of_edges)
					delete[] edges;
				edges = new Edge[number];
				number_of_edges = static_cast<int16_t>(number);
				is_owning = true;
			}
			void setEdges(Edge *ptr, int number) noexcept
			{
				assert(number >= 0 && number < std::numeric_limits<int16_t>::max());
				if (is_owning)
					delete[] edges;
				edges = ptr;
				number_of_edges = static_cast<int16_t>(number);
				is_owning = false;
			}
			void freeEdges() noexcept
			{
				if (is_owning)
					delete[] edges;
				number_of_edges = 0;
				edges = nullptr;
				is_owning = false;
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
				assert(win_rate >= -0.001f && win_rate <= 1.001f);
				assert(draw_rate >= -0.001f && draw_rate <= 1.001f);
				assert((win_rate + draw_rate) <= 1.001f);
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

			void removeEdge(int index)
			{
				assert(is_owning); // removing edges of non-owning nodes (in the tree) might not be the best idea, even if it's correctly implemented (which I'm not sure)
				assert(index >= 0 && index < number_of_edges);
				std::swap(edges[index], edges[number_of_edges - 1]);
				number_of_edges--;
			}

			std::string toString() const;
			void sortEdges() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODE_HPP_ */
