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
#include <cstring>
#include <string>
#include <cassert>

namespace ag
{

	class Edge
	{
			static constexpr uint16_t is_being_expanded = 0x8000u;

//			static constexpr int nb_bins = 10;
//			int histogram[nb_bins];
//			float variance = 0.0f;

			float policy_prior = 0.0f;
			Value value;
			int32_t visits = 0;
			Move move;
			Score score;
			uint16_t flag_and_virtual_loss = 0u;

			void set_flag(bool b) noexcept
			{
				flag_and_virtual_loss = b ? (flag_and_virtual_loss | is_being_expanded) : (flag_and_virtual_loss & (~is_being_expanded));
			}
			void set_virtual_loss(int vl) noexcept
			{
				flag_and_virtual_loss = (flag_and_virtual_loss & is_being_expanded) | (vl & (~is_being_expanded));
			}
		public:
			Edge() noexcept
			{
//				std::memset(histogram, 0, sizeof(int) * nb_bins);
			}
			Edge(Move m) noexcept :
					move(m)
			{
//				std::memset(histogram, 0, sizeof(int) * nb_bins);
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
			float getVariance() const noexcept
			{
//				assert(getVisits() >= 2);
//				return variance / static_cast<float>(getVisits() - 1);
				return 0.0;
			}
			float getExpectation() const noexcept
			{
				return getValue().getExpectation();
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
				return flag_and_virtual_loss & 0x7FFFu;
			}
			bool isBeingExpanded() const noexcept
			{
				return flag_and_virtual_loss & is_being_expanded;
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
//				const float delta = eval.getExpectation() - value.getExpectation();

				value += (eval - value) * tmp;
				value.clipToBounds();
//				variance += delta * (eval.getExpectation() - value.getExpectation());
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
				assert(getVirtualLoss() < 32768);
				set_virtual_loss(getVirtualLoss() + 1);
			}
			void decreaseVirtualLoss() noexcept
			{
				assert(getVirtualLoss() > 0);
				set_virtual_loss(getVirtualLoss() - 1);
			}
			void clearVirtualLoss() noexcept
			{
				set_virtual_loss(0);
			}
			void clearFlags() noexcept
			{
				set_flag(false);
			}
			void markAsBeingExpanded() noexcept
			{
				set_flag(true);
			}

			std::string toString() const;
			float sampleFromDistribution() const noexcept;
			void updateDistribution(Value eval) noexcept;
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
				if ((lhs.isProven() or rhs.isProven()) and lhs.getScore() != rhs.getScore())
					return lhs.getScore() > rhs.getScore();
				else
					return op(lhs) > op(rhs);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGE_HPP_ */
