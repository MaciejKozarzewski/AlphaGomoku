/*
 * GameGenerator.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_

#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>

namespace ag
{
	class GeneratorManager;
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
			GeneratorManager &manager;
			NNEvaluator &nn_evaluator;
			OpeningGenerator opening_generator;
			Game game;
			GameDataStorage game_data_storage;

			Tree tree;
			Search search;

			GameState state = GAME_NOT_STARTED;

			SelfplayConfig selfplay_config;
		public:
			enum Status
			{
				OK,
				TASKS_NOT_READY
			};

			GameGenerator(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, GeneratorManager &manager, NNEvaluator &evaluator);

			void clearStats();
			NodeCacheStats getCacheStats() const noexcept;
			SearchStats getSearchStats() const noexcept;

			void reset();
			Status generate();
			Json save(SerializedObject &binary_data);
			void load(const Json &json, const SerializedObject &binary_data);
		private:
			void make_move();
			void prepare_search();
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEGENERATOR_HPP_ */
