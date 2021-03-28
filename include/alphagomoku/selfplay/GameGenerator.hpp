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
				GAME_NOT_STARTED, PREPARE_OPENING, GAMEPLAY, SEND_RESULTS
			};
			GameBuffer &game_buffer;
			EvaluationQueue &queue;
			Game game;
			EvaluationRequest request;
			bool is_request_scheduled = false;

			Tree tree;
			Cache cache;
			Search search;

			GameState state = GAME_NOT_STARTED;
			int opening_trials = 0;

			int simulations = 0;
			float temperature = 0.0f;
			bool use_opening = false;
		public:
			GameGenerator(const Json &options, GameBuffer &gameBuffer, EvaluationQueue &queue);

			void clearStats();
			TreeStats getTreeStats() const noexcept;
			CacheStats getCacheStats() const noexcept;
			SearchStats getSearchStats() const noexcept;

			void reset();
			bool prepareOpening();
			void makeMove();
			bool performSearch();
			void generate();
		private:
			void prepare_search(const matrix<Sign> &board, Move lastMove);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_ */
