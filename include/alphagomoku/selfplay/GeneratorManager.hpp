/*
 * GeneratorManager.hpp
 *
 *  Created on: Mar 22, 2021
 *      Author: maciek
 */

#ifndef SELFPLAY_GENERATORMANAGER_HPP_
#define SELFPLAY_GENERATORMANAGER_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/GameGenerator.hpp>
#include <alphagomoku/utils/ThreadPool.hpp>
#include <inttypes.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

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

	class GeneratorThread: public Job
	{
		private:
			GeneratorManager &manager;
			EvaluationQueue queue;
			std::vector<std::unique_ptr<GameGenerator>> generators;
			ml::Device device;
			int batch_size;
		public:
			GeneratorThread(GeneratorManager &manager, const Json &options, ml::Device device);
			void run();
			void clearStats() noexcept;
			QueueStats getQueueStats() const noexcept;
			TreeStats getTreeStats() const noexcept;
			CacheStats getCacheStats() const noexcept;
			SearchStats getSearchStats() const noexcept;
	};

	class GeneratorManager
	{
		private:
			ThreadPool thread_pool;
			std::vector<std::unique_ptr<GeneratorThread>> generators;
			GameBuffer game_buffer;

			int games_to_generate = 0;
			std::string path_to_network;

		public:
			GeneratorManager(const Json &options);

			const GameBuffer& getGameBuffer() const noexcept;
			GameBuffer& getGameBuffer() noexcept;
			std::string getPathToNetwork() const;

			void generate(const std::string &pathToNetwork, int numberOfGames);
			bool hasEnoughGames() const noexcept;

			void printStats();
	};

} /* namespace ag */

#endif /* SELFPLAY_GENERATORMANAGER_HPP_ */
