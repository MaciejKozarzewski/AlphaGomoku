/*
 * EvaluationGame.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	class NNEvaluator;
} /* namespace ag */

namespace ag
{

	class Player
	{
		private:
			NNEvaluator &nn_evaluator;
			GameConfig game_config;
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
			void scheduleSingleTask(SearchTask &task);
			Move getMove() const noexcept;
			SearchData getSearchData() const;
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
			GameBuffer &game_buffer;
			Game game;
			SearchTask request;
			bool is_request_scheduled = false;

			GameState state = GAME_NOT_STARTED;
			int opening_trials = 0;
			bool use_opening = false;
			bool save_data = false;

			std::vector<Move> opening;
			bool has_stored_opening = false;

			std::unique_ptr<Player> first_player;
			std::unique_ptr<Player> second_player;
		public:
			EvaluationGame(GameConfig gameConfig, GameBuffer &gameBuffer, bool useOpening, bool saveData);
			void clear();
			void setFirstPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			bool prepareOpening();
			void generate();
		private:
			Player& get_player() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_ */
