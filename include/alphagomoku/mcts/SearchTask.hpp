/*
 * SearchTask.hpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_
#define ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace ag
{
	/**
	 * \brief Combines Node and Edge selected in each step of select phase.
	 * Node and Edge must always be in pairs.
	 */
	struct NodeEdgePair
	{
			Node *node = nullptr; // non-owning
			Edge *edge = nullptr; // non-owning
	};
	/**
	 *\brief Collects together all information required in a single step of the tree search.
	 */
	class SearchTask
	{
		private:
			std::vector<NodeEdgePair> visited_path;/**< trajectory within the tree visited during select phase */
			std::vector<Edge> proven_edges; /**< provably winning edges found by VCF solver */
			std::vector<Edge> edges; /**< edges created by EdgeGenerator */

			GameRules game_rules;
			matrix<Sign> board; /**< board representation of a state at the end of visited_path */
			Sign sign_to_move = Sign::NONE;

			matrix<float> policy; /**< policy returned from neural network */
			matrix<Value> action_values; /**< matrix of action values returned from neural network */
			Value value; /**< value returned from neural network */

			bool is_ready = false; /**< flag indicating whether the task has been evaluated and can be used for edge generation */
		public:
			SearchTask(GameRules rules);
			void reset(const matrix<Sign> &base, Sign signToMove) noexcept;

			int visitedPathLength() const noexcept
			{
				return static_cast<int>(visited_path.size());
			}
			NodeEdgePair getPair(int index) const noexcept
			{
				assert(index >= 0 && index < visitedPathLength());
				return visited_path[index];
			}
			NodeEdgePair getLastPair() const noexcept
			{
				assert(visitedPathLength() > 0);
				return visited_path.back();
			}
			Edge* getLastEdge() const noexcept
			{
				if (visited_path.size() == 0)
					return nullptr;
				else
					return visited_path.back().edge;
			}

			const std::vector<Edge>& getProvenEdges() const noexcept
			{
				return proven_edges;
			}
			std::vector<Edge>& getProvenEdges() noexcept
			{
				return proven_edges;
			}
			const std::vector<Edge>& getEdges() const noexcept
			{
				return edges;
			}
			std::vector<Edge>& getEdges() noexcept
			{
				return edges;
			}
			GameRules getGameRules() const noexcept
			{
				return game_rules;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			Value getValue() const noexcept
			{
				return value;
			}
			bool isReady() const noexcept
			{
				return is_ready;
			}
			const matrix<Sign>& getBoard() const noexcept
			{
				return board;
			}
			matrix<Sign>& getBoard() noexcept
			{
				return board;
			}
			const matrix<Value>& getActionValues() const noexcept
			{
				return action_values;
			}
			matrix<Value>& getActionValues() noexcept
			{
				return action_values;
			}
			const matrix<float>& getPolicy() const noexcept
			{
				return policy;
			}
			matrix<float>& getPolicy() noexcept
			{
				return policy;
			}

			void append(Node *node, Edge *edge);
			void markAsReady() noexcept
			{
				is_ready = true;
			}

			/**
			 * \param[in] p Pointer to the memory block containing calculated policy.
			 */
			void setPolicy(const float *p) noexcept;

			/**
			 * \param[in] q Pointer to the memory block containing calculated action values.
			 */
			void setActionValues(const float *q) noexcept;
			void setValue(Value v) noexcept
			{
				value = v;
			}

			void addProvenEdge(Move move, ProvenValue pv);
			void addEdge(Move move);
			void addEdge(const Edge &other)
			{
				edges.push_back(other);
			}

			std::string toString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_ */
