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
	 * @brief Combines Node and Edge selected in each step of select phase.
	 * Node and Edge must always be in pairs.
	 */
	struct NodeEdgePair
	{
			Node *node = nullptr; // non-owning
			Edge *edge = nullptr; // non-owning
	};
	/**
	 * @brief Collects together all information required in a single step of the tree search.
	 */
	class SearchTask
	{
		private:
			std::vector<NodeEdgePair> visited_path;/**< trajectory within the tree visited during select phase */
			std::vector<Edge> proven_edges; /**< provably winning edges found by VCF solver */
			std::vector<Edge> edges; /**< edges created by EdgeGenerator */

			matrix<Sign> board; /**< board representation of a state at the end of visited_path */
			Sign sign_to_move = Sign::NONE;

			matrix<float> policy; /**< policy returned from neural network */
			matrix<Value> action_values; /**< matrix of action values returned from neural network */
			Value value; /**< value returned from neural network */

			bool is_ready = false; /**< flag indicating whether the task has been evaluated and can be used for edge generation */
		public:
			void reset(const matrix<Sign> &base, Sign signToMove) noexcept;

			int visitedPathLength() const noexcept;

			NodeEdgePair getPair(int index) const noexcept;
			NodeEdgePair getLastPair() const noexcept;

			const std::vector<Edge>& getProvenEdges() const noexcept;
			std::vector<Edge>& getProvenEdges() noexcept;

			const std::vector<Edge>& getEdges() const noexcept;
			std::vector<Edge>& getEdges() noexcept;

			Sign getSignToMove() const noexcept;
			Value getValue() const noexcept;
			bool isReady() const noexcept;

			const matrix<Sign>& getBoard() const noexcept;
			matrix<Sign>& getBoard() noexcept;

			const matrix<Value>& getActionValues() const noexcept;
			matrix<Value>& getActionValues() noexcept;

			const matrix<float>& getPolicy() const noexcept;
			matrix<float>& getPolicy() noexcept;

			void append(Node *node, Edge *edge);
			void setReady() noexcept;

			/**
			 * \param[in] p Pointer to the memory block containing calculated policy.
			 */
			void setPolicy(const float *p) noexcept;

			/**
			 * \param[in] q Pointer to the memory block containing calculated action values.
			 */
			void setActionValues(const float *q) noexcept;
			void setValue(Value v) noexcept;

			void addProvenEdge(Move move, ProvenValue pv);
			void addEdge(Move move);
			void addEdge(const Edge &other);

			std::string toString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_ */
