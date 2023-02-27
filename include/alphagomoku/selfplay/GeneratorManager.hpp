/*
 * GeneratorManager.hpp
 *
 *  Created on: Mar 22, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef SELFPLAY_GENERATORMANAGER_HPP_
#define SELFPLAY_GENERATORMANAGER_HPP_

#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>

#include <cinttypes>
#include <string>
#include <future>

namespace ag
{
	class GeneratorManager;
} /* namespace ag */

namespace ag
{
	struct SelfPlayStats
	{
			//NNQueue stats
			uint64_t network_evals = 0;
			uint64_t evaluated_positions = 0;

			//Tree stats
			uint64_t avg_used_nodes = 0;
			uint64_t max_used_nodes = 0;

			//NNCache stats
			uint64_t cache_calls = 0;
			uint64_t cache_hits = 0;

			std::string toString() const
			{
				std::string result = std::to_string(network_evals) + " " + std::to_string(evaluated_positions);
				result += std::string(" ") + std::to_string(avg_used_nodes) + " " + std::to_string(max_used_nodes);
				result += std::string(" ") + std::to_string(cache_calls) + " " + std::to_string(cache_hits);
				return result;
			}
	};

	class GeneratorThread
	{
		private:
			std::future<void> generator_future;
			std::atomic<bool> is_running;

			GeneratorManager &manager;
			NNEvaluator nn_evaluator;
			std::vector<std::unique_ptr<GameGenerator>> generators;
		public:
			GeneratorThread(GeneratorManager &manager, const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions, int index);
			void start();
			void stop();
			bool isFinished() const noexcept;
			void clearStats() noexcept;
			void resetGames();
			NNEvaluatorStats getEvaluatorStats() const noexcept;
			NodeCacheStats getCacheStats() const noexcept;
			SearchStats getSearchStats() const noexcept;
			void saveGames(const std::string &path) const;
			void loadGames(const std::string &path);
		private:
			void run();
	};

	class GeneratorManager
	{
		private:
			mutable std::mutex buffer_mutex;
			std::vector<std::unique_ptr<GeneratorThread>> generators;
			GameDataBuffer game_buffer;

			int games_to_generate = 0;
			std::string path_to_network;
			std::string working_directory;
		public:
			GeneratorManager(const GameConfig &gameOptions, const SelfplayConfig &selfplayOptions);

			void setWorkingDirectory(const std::string &path);
			void addToBuffer(const GameDataStorage &gameData);

			const GameDataBuffer& getGameBuffer() const noexcept;
			GameDataBuffer& getGameBuffer() noexcept;
			std::string getPathToNetwork() const;

			void resetGames();
			void generate(const std::string &pathToNetwork, int numberOfGames);
			bool hasEnoughGames() const noexcept;

			void printStats();
		private:
			void save_state(bool saveBuffer);
			void load_state();
	};

} /* namespace ag */

#endif /* SELFPLAY_GENERATORMANAGER_HPP_ */
