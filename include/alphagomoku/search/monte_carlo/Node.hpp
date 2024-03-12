/*
 * Node.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODE_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODE_HPP_

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/search/Value.hpp>

#include <string>
#include <cinttypes>
#include <cmath>
#include <cassert>

namespace ag
{
	class Node
	{
			static constexpr uint16_t is_owning = 0x0001u;
			static constexpr uint16_t is_root = 0x0002u;
			static constexpr uint16_t is_fully_expanded = 0x0004u;

			Edge *edges = nullptr; // may be owning or not
			Value value;
			float variance = 0.0f;
			int32_t visits = 0;
			Score score;
			int16_t number_of_edges = 0;
			int16_t depth = 0;
			int16_t virtual_loss = 0;
			Sign sign_to_move = Sign::NONE;
			uint16_t flags = 0; // from least significant bit: is_owning, is_root, is_fully_expanded

			template<uint16_t F>
			void set_flag(bool value) noexcept
			{
				flags = value ? (flags | F) : (flags & (~F));
			}
			template<uint16_t F>
			bool get_flag() const noexcept
			{
				return (flags & F) != 0;
			}
		public:
			Node() noexcept = default;
			Node(const Node &other) noexcept :
					edges(nullptr),
					value(other.value),
					variance(other.variance),
					visits(other.visits),
					score(other.score),
					number_of_edges(0),
					depth(other.depth),
					virtual_loss(other.virtual_loss),
					sign_to_move(other.sign_to_move),
					flags(other.flags)
			{
				set_flag<is_owning>(false);
			}
			Node(Node &&other) noexcept :
					edges(other.edges),
					value(other.value),
					variance(other.variance),
					visits(other.visits),
					score(other.score),
					number_of_edges(other.number_of_edges),
					depth(other.depth),
					virtual_loss(other.virtual_loss),
					sign_to_move(other.sign_to_move),
					flags(other.flags)
			{
				other.edges = nullptr;
				other.number_of_edges = 0;
				other.set_flag<is_owning>(false);
			}
			Node& operator=(const Node &other) noexcept
			{
				edges = nullptr;
				value = other.value;
				variance = other.variance;
				visits = other.visits;
				score = other.score;
				number_of_edges = 0;
				depth = other.depth;
				virtual_loss = other.virtual_loss;
				sign_to_move = other.sign_to_move;
				flags = other.flags;
				set_flag<is_owning>(false);
				return *this;
			}
			Node& operator=(Node &&other) noexcept
			{
				std::swap(this->edges, other.edges);
				std::swap(this->value, other.value);
				std::swap(this->variance, other.variance);
				std::swap(this->visits, other.visits);
				std::swap(this->score, other.score);
				std::swap(this->number_of_edges, other.number_of_edges);
				std::swap(this->depth, other.depth);
				std::swap(this->virtual_loss, other.virtual_loss);
				std::swap(this->sign_to_move, other.sign_to_move);
				std::swap(this->flags, other.flags);
				return *this;
			}
			~Node() noexcept
			{
				if (isOwning())
					delete[] edges;
			}

			void clear() noexcept
			{
				value = Value();
				visits = 0;
				score = Score();
				depth = 0;
				virtual_loss = 0;
				sign_to_move = Sign::NONE;
				set_flag<is_root>(false);
				set_flag<is_fully_expanded>(false);
			}

			bool isOwning() const noexcept
			{
				return get_flag<is_owning>();
			}
			bool isRoot() const noexcept
			{
				return get_flag<is_root>();
			}
			bool isFullyExpanded() const noexcept
			{
				return get_flag<is_fully_expanded>();
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
			float getVariance() const noexcept
			{
				assert(getVisits() >= 2);
				return variance / static_cast<float>(getVisits() - 1);
			}
			float getExpectation() const noexcept
			{
				return getValue().getExpectation();
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
			ProvenValue getProvenValue() const noexcept
			{
				return score.getProvenValue();
			}
			int numberOfEdges() const noexcept
			{
				return number_of_edges;
			}
			int getDepth() const noexcept
			{
				return depth;
			}
			int getVirtualLoss() const noexcept
			{
				return virtual_loss;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}

			void markAsRoot() noexcept
			{
				set_flag<is_root>(true);
			}
			void markAsFullyExpanded() noexcept
			{
				set_flag<is_fully_expanded>(true);
			}
			void createEdges(int number)
			{
				assert(0 <= number && number < std::numeric_limits<int16_t>::max());
				if (isOwning() and number != number_of_edges)
					delete[] edges;
				edges = new Edge[number];
				number_of_edges = static_cast<int16_t>(number);
				set_flag<is_owning>(true);
			}
			void setEdges(Edge *ptr, int number) noexcept
			{
				assert(0 <= number && number < std::numeric_limits<int16_t>::max());
				if (isOwning())
					delete[] edges;
				edges = ptr;
				number_of_edges = static_cast<int16_t>(number);
				set_flag<is_owning>(false);
			}
			void freeEdges() noexcept
			{
				if (isOwning())
					delete[] edges;
				number_of_edges = 0;
				edges = nullptr;
				set_flag<is_owning>(false);
			}
			void setValue(Value value) noexcept
			{
				assert(value.isValid());
				this->value = value;
			}
			void updateValue(Value eval) noexcept
			{
				visits++;
				const float tmp = 1.0f / static_cast<float>(visits);
				const float delta = eval.getExpectation() - value.getExpectation();

				value += (eval - value) * tmp;
				value.clipToBounds();
				variance += delta * (eval.getExpectation() - value.getExpectation());
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
				assert(0 <= d && d < std::numeric_limits<int16_t>::max());
				depth = d;
			}
			void increaseVirtualLoss() noexcept
			{
				assert(virtual_loss < std::numeric_limits<int16_t>::max());
				virtual_loss += 1;
			}
			void decreaseVirtualLoss() noexcept
			{
				assert(virtual_loss > 0);
				virtual_loss -= 1;
			}
			void clearVirtualLoss() noexcept
			{
				virtual_loss = 0;
			}
			void setSignToMove(Sign s) noexcept
			{
				sign_to_move = s;
			}

			void removeEdge(int index) noexcept
			{
				assert(isOwning()); // removing edges of non-owning nodes (in the tree) might not be the best idea, even if it's correctly implemented (which I'm not sure)
				assert(0 <= index && index < number_of_edges);
				std::swap(edges[index], edges[number_of_edges - 1]);
				number_of_edges--;
			}

			std::string toString() const;
			void sortEdges() const;
	};

	void copyEdgeInfo(Node &dst, const Node &src);

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODE_HPP_ */
