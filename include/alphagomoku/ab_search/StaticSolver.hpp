/*
 * StaticSolver.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef AB_SEARCH_STATICSOLVER_HPP_
#define AB_SEARCH_STATICSOLVER_HPP_

#include <alphagomoku/ab_search/Score.hpp>
#include <alphagomoku/ab_search/CommonBase.hpp>
#include <alphagomoku/utils/configs.hpp>

namespace ag
{
	class ActionList;
}

namespace ag
{
	class StaticSolver: CommonBase
	{
			struct SolverResult
			{
					bool can_continue = true;
					Score score;
					SolverResult() noexcept = default;
					SolverResult(bool c, Score s) noexcept :
							can_continue(c),
							score(s)
					{
					}
			};
			int number_of_moves_for_draw;
		public:
			StaticSolver(GameConfig gameConfig, PatternCalculator &calc);
			void setDrawAfter(int moves) noexcept;
			Score solve(ActionList &actions, int depth = 5);
		private:
			SolverResult check_win_in_1(ActionList &actions);
			SolverResult check_draw_in_1(ActionList &actions);
			SolverResult check_loss_in_2(ActionList &actions);
			SolverResult check_win_in_3(ActionList &actions);
			SolverResult check_win_in_5(ActionList &actions);
			Score try_solve_fork_4x3(Move move);
	};

} /* namespace ag */

#endif /* AB_SEARCH_STATICSOLVER_HPP_ */
