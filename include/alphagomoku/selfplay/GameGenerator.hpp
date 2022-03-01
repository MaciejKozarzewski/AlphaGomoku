/*
 * GameGenerator.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

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
				GAME_NOT_STARTED,
				PREPARE_OPENING,
				GAMEPLAY_SELECT_SOLVE_EVALUATE,
				GAMEPLAY_EXPAND_AND_BACKUP
			};
			GameBuffer &game_buffer;
			NNEvaluator &nn_evaluator;
			Game game;
			SearchTask request;
			bool is_request_scheduled = false;

			Tree tree;
			Search search;

			GameState state = GAME_NOT_STARTED;
			int opening_trials = 0;

			int simulations_min = 0;
			int simulations_max = 0;
			int positions_skip = 1;
			bool use_opening = false;
			bool perform_full_search = false;
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
			void clear_node_cache();
			int nb_of_simulations() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_ */
