/*
 * SearchTrajectory.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCHTRAJECTORY_HPP_
#define ALPHAGOMOKU_MCTS_SEARCHTRAJECTORY_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <cassert>
#include <string>
#include <vector>

namespace ag
{
	class SearchTrajectory
	{
		private:
			std::vector<Node*> visited_nodes;
			std::vector<Move> visited_moves;
		public:
			void clear() noexcept
			{
				visited_nodes.clear();
				visited_moves.clear();
			}
			int length() const noexcept
			{
				return static_cast<int>(visited_nodes.size());
			}
			const Node& getNode(int index) const noexcept
			{
				assert(index >= 0 && index < length());
				return *(visited_nodes[index]);
			}
			Node& getNode(int index) noexcept
			{
				assert(index >= 0 && index < length());
				return *(visited_nodes[index]);
			}
			Move getMove(int index) const noexcept
			{
				assert(index >= 0 && index < length());
				return visited_moves[index];
			}
			void append(Node *node, Move move)
			{
				visited_nodes.push_back(node);
				visited_moves.push_back(move);
			}
			Node& getLeafNode() const noexcept
			{
				assert(length() > 0);
				return *(visited_nodes.back());
			}
			Move getLastMove() const noexcept
			{
				assert(length() > 0);
				return visited_moves.back();
			}
			std::string toString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCHTRAJECTORY_HPP_ */
