/*
 * Node.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_NODE_HPP_
#define ALPHAGOMOKU_MCTS_NODE_HPP_

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/search/Value.hpp>

#include <string>
#include <cassert>
#include <cinttypes>
#include <cmath>

namespace ag
{
	class Node
	{
		private:
			Edge *edges = nullptr;
			Value value;
			int32_t visits = 0;
			Score score;
			int16_t number_of_edges = 0;
			int16_t depth = 0;
			Sign sign_to_move = Sign::NONE;
			bool is_owning = false;
			bool is_root = false;
		public:
			Node() = default;
			Node(const Node &other) :
					value(other.value),
					visits(other.visits),
					score(other.score),
					number_of_edges(0),
					depth(other.depth),
					sign_to_move(other.sign_to_move)
			{
			}
			Node(Node &&other) noexcept :
					edges(other.edges),
					value(other.value),
					visits(other.visits),
					score(other.score),
					number_of_edges(other.number_of_edges),
					depth(other.depth),
					sign_to_move(other.sign_to_move),
					is_owning(other.is_owning),
					is_root(other.is_root)
			{
				other.edges = nullptr;
				other.number_of_edges = 0;
				other.is_owning = false;
			}
			Node& operator=(const Node &other)
			{
				edges = nullptr;
				value = other.value;
				visits = other.visits;
				score = other.score;
				number_of_edges = 0;
				depth = other.depth;
				sign_to_move = other.sign_to_move;
				is_owning = false;
				is_owning = other.is_root;
				return *this;
			}
			Node& operator=(Node &&other) noexcept
			{
				std::swap(this->edges, other.edges);
				std::swap(this->value, other.value);
				std::swap(this->visits, other.visits);
				std::swap(this->score, other.score);
				std::swap(this->number_of_edges, other.number_of_edges);
				std::swap(this->depth, other.depth);
				std::swap(this->sign_to_move, other.sign_to_move);
				std::swap(this->is_owning, other.is_owning);
				std::swap(this->is_root, other.is_root);
				return *this;
			}
			~Node()
			{
				if (is_owning)
					delete[] edges;
			}

			void clear() noexcept
			{
				value = Value();
				visits = 0;
				score = Score();
				depth = 0;
				sign_to_move = Sign::NONE;
				is_root = false;
			}

			bool isRoot() const noexcept
			{
				return is_root;
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
				return value.win_rate;
			}
			float getDrawRate() const noexcept
			{
				return value.draw_rate;
			}
			float getLossRate() const noexcept
			{
				return value.loss_rate();
			}
			Value getValue() const noexcept
			{
				return value;
			}
			int getVisits() const noexcept
			{
				return visits;
			}
			Score getScore() const noexcept
			{
				return score;
			}
			bool isProven() const noexcept
			{
				return score.isProven();
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

			void markAsRoot() noexcept
			{
				is_root = true;
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
				this->value = value;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				value += (eval - value) * tmp;
				assert(value.isValid());
			}
			void setScore(Score s) noexcept
			{
				score = s;
			}
			void updateScore(Score s) noexcept
			{
				score = std::max(score, s);
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
