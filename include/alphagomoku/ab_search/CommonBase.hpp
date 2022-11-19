/*
 * CommonBase.hpp
 *
 *  Created on: Oct 6, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_COMMONBASE_HPP_
#define ALPHAGOMOKU_AB_SEARCH_COMMONBASE_HPP_

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

namespace ag
{
	class CommonBase
	{
		private:
			std::vector<Location> internal_temporary_list;
			std::vector<Location> *external_temporary_list = nullptr;
		protected:
			GameConfig game_config;
			PatternCalculator &pattern_calculator;
		public:
			CommonBase(GameConfig gameConfig, PatternCalculator &calc) :
					game_config(gameConfig),
					pattern_calculator(calc)
			{
			}
		protected:
			bool is_caro_rule() const noexcept
			{
				return game_config.rules == GameRules::CARO5 or game_config.rules == GameRules::CARO6;
			}
			Sign get_own_sign() const noexcept
			{
				return pattern_calculator.getSignToMove();
			}
			Sign get_opponent_sign() const noexcept
			{
				return invertSign(get_own_sign());
			}
			ThreatType get_own_threat_at(int row, int col) const noexcept
			{
				return pattern_calculator.getThreatAt(get_own_sign(), row, col);
			}
			ThreatType get_opponent_threat_at(int row, int col) const noexcept
			{
				return pattern_calculator.getThreatAt(get_opponent_sign(), row, col);
			}
			const std::vector<Location>& get_own_threats(ThreatType tt) const noexcept
			{
				return pattern_calculator.getThreatHistogram(get_own_sign()).get(tt);
			}
			const std::vector<Location>& get_opponent_threats(ThreatType tt) const noexcept
			{
				return pattern_calculator.getThreatHistogram(get_opponent_sign()).get(tt);
			}
			bool is_own_4x4_fork_forbidden() const noexcept
			{
				return is_anything_forbidden_for(get_own_sign());
			}
			bool is_opponent_4x4_fork_forbidden() const noexcept
			{
				return is_anything_forbidden_for(get_opponent_sign());
			}
			bool is_own_3x3_fork_forbidden(Location m) const noexcept
			{
				if (is_anything_forbidden_for(get_own_sign()))
					return pattern_calculator.isForbidden(get_own_sign(), m.row, m.col);
				return false;
			}
			bool is_opponent_3x3_fork_forbidden(Location m) const noexcept
			{
				if (is_anything_forbidden_for(get_opponent_sign()))
					return pattern_calculator.isForbidden(get_opponent_sign(), m.row, m.col);
				return false;
			}
			bool is_anything_forbidden_for(Sign sign) const noexcept
			{
				return (game_config.rules == GameRules::RENJU) and (sign == Sign::CROSS);
			}
			void set_external_temporary_list(std::vector<Location> &tmp) noexcept
			{
				external_temporary_list = &tmp;
			}
			std::vector<Location>& get_temporary_list()
			{
				if (external_temporary_list != nullptr)
				{
					external_temporary_list->clear();
					return *external_temporary_list;
				}
				else
				{
					internal_temporary_list.clear();
					return internal_temporary_list;
				}
			}
			const std::vector<Location>& get_copy(const std::vector<Location> &list)
			{
				std::vector<Location> &tmp = get_temporary_list();
				tmp = list;
				return tmp;
			}
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_COMMONBASE_HPP_ */
