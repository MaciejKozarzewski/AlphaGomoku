/*
 * GameGenerator.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/Search.hpp>

class Json;
namespace ag
{
	class GameBuffer;
} /* namespace ag */

namespace ag
{

	class GameGenerator
	{
		private:
			enum GameState
			{
				GAME_NOT_STARTED, PREPARE_OPENING, GAMEPLAY, GAME_ENDED, SEND_RESULTS
			};
			GameBuffer &game_buffer;
			EvaluationQueue &queue;
			Game game;
			EvaluationRequest request;
			bool is_request_scheduled = false;

			std::unique_ptr<Tree> tree;
			std::unique_ptr<Cache> cache;
			std::unique_ptr<Search> search;

			GameState state = GAME_NOT_STARTED;
			int opening_trials = 0;
			GeneratorConfig config;
		public:
			GameGenerator(GameConfig gameOptions, GameBuffer &gameBuffer, EvaluationQueue &queue);

			void init(const Json &selfplayOptions);

			void clearStats();
			SearchStats getSearchStats() const;
			bool prepareOpening();
			void makeMove(const matrix<float> &policy);
			bool performSearch(int iterations);
			void generate();
		private:
			void prepare_search(const matrix<Sign> &board, Move lastMove);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_ */
