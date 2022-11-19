/*
 * MinimaxSearch.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_MINIMAXSEARCH_HPP_
#define ALPHAGOMOKU_AB_SEARCH_MINIMAXSEARCH_HPP_

#include <alphagomoku/ab_search/MoveGenerator.hpp>
#include <alphagomoku/ab_search/ActionList.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class SearchTask;
}

namespace ag
{
	/*
	 * \brief Reference implementation, used mostly for testing.
	 */
	class MinimaxSearch
	{
			GameConfig game_config;
			PatternCalculator pattern_calculator;
			ActionStack action_stack;
			matrix<Sign> current_board;
		public:
			MinimaxSearch(GameConfig gameConfig);
			std::pair<Move, Score> solve(SearchTask &task, int depth);
		private:
			Score recursive_solve(int depthRemaining, ActionList &actions);
			Score evaluate();
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_MINIMAXSEARCH_HPP_ */
