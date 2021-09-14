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
	struct NodeEdgePair
	{
			Node *node = nullptr; // non-owning
			Edge *edge = nullptr; // non-owning
	};
	class SearchTask
	{
		private:
			std::vector<NodeEdgePair> visited_pairs;
			Board board;
			Move last_move;
			matrix<float> policy;
			Value value;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			bool is_ready = false;
		public:
			void reset(const Board &base) noexcept;
			void correctInformationLeak() noexcept;
			void chackIfTerminal() noexcept;
			int size() const noexcept
			{
				return static_cast<int>(visited_pairs.size());
			}
			NodeEdgePair getPair(int index) const noexcept
			{
				assert(index >= 0 && index < length());
				return visited_pairs[index];
			}
			void append(Node *node, Edge *edge)
			{
				assert(node != nullptr);
				assert(edge != nullptr);
				visited_pairs.push_back( { node, edge });
				last_move = edge->getMove();
				board.putMove(last_move);
			}
			NodeEdgePair getLastPair() const noexcept
			{
				assert(size() > 0);
				return visited_pairs.back();
			}
			Sign getSignToMove() const noexcept
			{
				return board.signToMove();
			}
			Value getValue() const noexcept
			{
				return value;
			}
			ProvenValue getProvenValue() const noexcept
			{
				return proven_value;
			}
			bool isReady() const noexcept
			{
				return is_ready;
			}
			const Board& getBoard() const noexcept
			{
				return board;
			}
			Board& getBoard() noexcept
			{
				return board;
			}
			const matrix<float>& getPolicy() const noexcept
			{
				return policy;
			}
			matrix<float>& getPolicy() noexcept
			{
				return policy;
			}

			void setReady() noexcept
			{
				is_ready = true;
			}
			void setPolicy(const float *p) noexcept
			{
				std::memcpy(policy.data(), p, policy.sizeInBytes());
			}
			void setValue(Value v) noexcept
			{
				value = v;
			}
			void setProvenValue(ProvenValue pv) noexcept
			{
				proven_value = pv;
			}
			std::string toString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_ */
