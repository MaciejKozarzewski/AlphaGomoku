/*
 * Edge.hpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGE_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGE_HPP_

#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/Move.hpp>

#include <cinttypes>
#include <string>
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
			Node *node = nullptr; // non-owning
			float policy_prior = 0.0f;
			Value value;
			int32_t visits = 0;
			Move move;
			Score score;
			int16_t virtual_loss = 0;
		public:

			Edge() noexcept = default;
			Edge(Move m) noexcept :
					move(m)
			{
			}

			bool isLeaf() const noexcept
			{
				return node == nullptr;
			}
			Node* getNode() const noexcept
			{
				return node;
			}
			float getPolicyPrior() const noexcept
			{
				return policy_prior;
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
			float getExpectation(float styleFactor = 0.5f) const noexcept
			{
				return getValue().getExpectation(styleFactor);
			}
			int getVisits() const noexcept
			{
				return visits;
			}
			Move getMove() const noexcept
			{
				return move;
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
			int getVirtualLoss() const noexcept
			{
				return virtual_loss;
			}

			void setNode(Node *ptr) noexcept
			{
				assert(ptr != nullptr);
				node = ptr;
			}
			void setPolicyPrior(float p) noexcept
			{
				policy_prior = p;
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
				value += (eval - value) * tmp;
				value.clipToBounds();
			}
			void setMove(Move m) noexcept
			{
				move = m;
			}
			void setScore(Score s) noexcept
			{
				score = s;
			}
			void updateScore(Score s) noexcept
			{
				score = std::max(score, s);
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

			std::string toString() const;
			Edge copyInfo() const noexcept;
	};

	template<class Op>
	struct EdgeComparator
	{
			Op op;
			EdgeComparator(Op o) noexcept :
					op(o)
			{
			}
			bool operator()(const Edge &lhs, const Edge &rhs) const noexcept
			{
				if (lhs.isProven() or rhs.isProven())
					return lhs.getScore() > rhs.getScore();
				else
					return op(lhs) > op(rhs);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGE_HPP_ */
