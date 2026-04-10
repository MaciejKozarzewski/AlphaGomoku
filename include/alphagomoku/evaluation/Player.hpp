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
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class NNEvaluator;
} /* namespace ag */

namespace ag
{

	enum class ConstrainType
	{
		SIMULATIONS,
		TIME
	};

	struct Constraints
	{
			double time_for_turn = 0.0;
			double time_for_match = 0.0;
			double time_increment = 0.0;

			int max_simulations = 0;

			ConstrainType type = ConstrainType::SIMULATIONS;

			static Constraints time_controls(double turn, double match, double incerement = 0.0) noexcept;
			static Constraints simulations(int max_sim) noexcept;
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
			int simulations = 0;
			int nn_evals = 0;
		public:
			Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			void setSign(Sign s) noexcept;
			Sign getSign() const noexcept;
			std::string getName() const;
			void setBoard(const matrix<Sign> &board, Sign signToMove);
			void selectSolveEvaluate();
			void expandBackup();
			bool isSearchOver();
			AlphaBetaSearch& getSolver() noexcept;
			NNEvaluator& getNNEvaluator() noexcept;
			void scheduleSingleTask(SearchTask &task);
			Move getMove() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_PLAYER_HPP_ */
