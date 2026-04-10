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
#include <alphagomoku/evaluation/TwoMatch.hpp>
#include <alphagomoku/evaluation/Player.hpp>
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
			TwoMatch match;
			OpeningGenerator opening_generator;

			GameState state = GAME_NOT_STARTED;
			bool use_opening = false;

			std::vector<Move> opening;

			std::unique_ptr<Player> first_player;
			std::unique_ptr<Player> second_player;
			int currently_played_game = 0;

		public:
			EvaluationGame(GameConfig gameConfig, EvaluatorThread &manager, bool useOpening);
			void clear();
			void setFirstPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			void setSecondPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name);
			bool prepareOpening();
			void generate(int p = 3);
		private:
			Player& get_player() noexcept;
			Game& get_game() noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_ */
