/*
 * GameGenerator.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>

namespace ag
{
	class GameBuffer;
	class NNEvaluator;
} /* namespace ag */

namespace ag
{

	class GameGenerator
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
			NNEvaluator &nn_evaluator;
			OpeningGenerator opening_generator;
			Game game;

			Tree tree;
			Search search;

			GameState state = GAME_NOT_STARTED;

			SelfplayConfig selfplay_config;
		public:
			GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GameBuffer &gameBuffer, NNEvaluator &evaluator);

			void clearStats();
			NodeCacheStats getCacheStats() const noexcept;
			SearchStats getSearchStats() const noexcept;

			void reset();
			void generate();
		private:
			bool prepare_opening();
			void make_move();
			void prepare_search(const matrix<Sign> &board, Sign signToMove);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_ */
