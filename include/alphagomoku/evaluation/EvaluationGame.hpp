/*
 * EvaluationGame.hpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_
#define ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_

#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/utils/matrix.hpp>

class Json;
namespace ag
{
	class EvaluationQueue;
} /* namespace ag */

namespace ag
{

	class Player
	{
		private:
			EvaluationQueue &queue;
			GameConfig game_config;
			Tree tree;
			Cache cache;
			Search search;

			int simulations = 0;
		public:
			Player(GameConfig gameOptions, const Json &options, EvaluationQueue &queue);
			void prepareSearch(const matrix<Sign> &board, Move lastMove);
			bool performSearch();
			void scheduleSingleRequest(EvaluationRequest &request);
			Move getMove() const noexcept;
	};

	class EvaluationGame
	{
		private:
			enum GameState
			{
				GAME_NOT_STARTED, PREPARE_OPENING, GAMEPLAY, SEND_RESULTS
			};
			GameBuffer &game_buffer;
			Game game;
			EvaluationRequest request;
			bool is_request_scheduled = false;

			GameState state = GAME_NOT_STARTED;
			int opening_trials = 0;
			bool use_opening = false;

			std::vector<Move> opening;
			bool has_stored_opening = false;

			std::unique_ptr<Player> cross_player;
			std::unique_ptr<Player> circle_player;
		public:
			EvaluationGame(GameConfig gameConfig, GameBuffer &gameBuffer, bool useOpening);
			void clear();
			void setCrossPlayer(const Json &options, EvaluationQueue &queue);
			void setCirclePlayer(const Json &options, EvaluationQueue &queue);
			bool prepareOpening();
			void generate();
		private:
			Player& get_player() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_EVALUATION_EVALUATIONGAME_HPP_ */
