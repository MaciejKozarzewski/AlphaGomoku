/*
 * MinimaxSearch.hpp
 *
 *  Created on: Jun 1, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_MINIMAXSEARCH_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_MINIMAXSEARCH_HPP_

#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>
#include <alphagomoku/search/alpha_beta/ActionList.hpp>
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
				MoveGenerator move_generator;
				ActionStack action_stack;
				MoveGeneratorMode generator_mode = MoveGeneratorMode::OPTIMAL;
			public:
				MinimaxSearch(GameConfig gameConfig);
				void solve(SearchTask &task, int depth, MoveGeneratorMode gen_mode);
			private:
				Score recursive_solve(int depthRemaining, ActionList &actions);
				Score evaluate();
		};

} /* namespace ag */


#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_MINIMAXSEARCH_HPP_ */
