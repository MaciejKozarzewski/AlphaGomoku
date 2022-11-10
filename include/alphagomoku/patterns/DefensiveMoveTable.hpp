/*
 * DefensiveMoveTable.hpp
 *
 *  Created on: Oct 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_
#define ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_

#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/utils/BitMask.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/game/Move.hpp>

#include <algorithm>
#include <cassert>

namespace ag
{
	enum class GameRules;
}

namespace ag
{
	/*
	 * \brief Returns list of moves that can be used to promote an open three into a four (intended only for cross in renju rule).
	 */
	BitMask1D<uint16_t> getOpenThreePromotionMoves(NormalPattern pattern) noexcept;

	class DefensiveMoveTable
	{
			template<typename T>
			struct SignPair
			{
					T for_cross = T { };
					T for_circle = T { };
					T get(Sign s) const noexcept
					{
						assert(s == Sign::CROSS || s == Sign::CIRCLE);
						return (s == Sign::CROSS) ? for_cross : for_circle;
					}
			};

			matrix<SignPair<BitMask1D<uint16_t>>> five_defense;
			matrix<SignPair<BitMask1D<uint16_t>>> open_four_defense;
			matrix<SignPair<BitMask1D<uint16_t>>> double_four_defense;
			GameRules game_rules;
		public:
			DefensiveMoveTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			BitMask1D<uint16_t> getMoves(ExtendedPattern pattern, Sign defenderSign, PatternType threatToDefend) const;
			static const DefensiveMoveTable& get(GameRules rules);
		private:
			void init_five();
			void init_open_four();
			void init_double_four();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_DEFENSIVEMOVETABLE_HPP_ */
