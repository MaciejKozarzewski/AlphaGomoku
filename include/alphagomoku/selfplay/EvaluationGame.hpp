/*
 * EvaluationGame.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class NNEvaluator;
	class EvaluatorThread;
} /* namespace ag */

namespace ag
{

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

	class EvaluationGame
	{
		private:
			enum GameState
			{
				GAME_NOT_STARTED,
				PREPARE_OPENING,
				GAMEPLAY_SELECT_SOLVE_EVALUATE,
				GAMEPLAY_EXPAND_AND_BACKUP
			};
			EvaluatorThread &manager_thread;
			Game game;
			OpeningGenerator opening_generator;

			GameState state = GAME_NOT_STARTED;
			bool use_opening = false;

			std::vector<Move> opening;
			bool has_stored_opening = false;

			std::unique_ptr<Player> first_player;
			std::unique_ptr<Player> second_player;
		public:
			EvaluationGame(GameConfig gameConfig, EvaluatorThread &manager, bool useOpening);
			void clear();
			void setFirstPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			bool prepareOpening();
			void generate(int p = 3);
		private:
			Player& get_player() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_ */
