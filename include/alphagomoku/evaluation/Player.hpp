/*
 * Player.hpp
 *
 *  Created on: Apr 9, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_PLAYER_HPP_
#define ALPHAGOMOKU_EVALUATION_PLAYER_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class NNEvaluator;
} /* namespace ag */

namespace ag
{

	class TimeController
	{
			double used_time_match = 0.0;
			double used_time_turn = 0.0;
		public:
			void new_game() noexcept
			{
				used_time_match = 0.0;
				used_time_turn = 0.0;
			}
			void new_turn() noexcept
			{
				used_time_turn = 0.0;
			}
			void add_time(double dt) noexcept
			{
				used_time_match += dt;
				used_time_turn += dt;
			}
			double time_turn() const noexcept
			{
				return used_time_turn;
			}
			double time_match() const noexcept
			{
				return used_time_match;
			}
	};

	class Player
	{
		private:
			NNEvaluator &nn_evaluator;
			GameConfig game_config;
			EdgeSelectorConfig final_move_selection_config;
			Tree tree;
			Search search;

			std::string name;
			Sign sign = Sign::NONE;

			Constraints constraints;
			TimeController time_controller;
			MovesLeftEstimator moves_left_estimator;
		public:
			Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			void setSign(Sign s) noexcept;
			Sign getSign() const noexcept;
			std::string getName() const;
			void newGame();
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void selectSolveEvaluate();
			void expandBackup();
			bool isSearchOver();
			AlphaBetaSearch& getSolver() noexcept;
			NNEvaluator& getNNEvaluator() noexcept;
			void scheduleSingleTask(SearchTask &task);
			Move getMove();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_PLAYER_HPP_ */
