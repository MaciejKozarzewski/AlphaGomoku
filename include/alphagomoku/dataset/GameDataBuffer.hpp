/*
 * GameDataBuffer.hpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_GAMEDATABUFFER_HPP_
#define ALPHAGOMOKU_DATASET_GAMEDATABUFFER_HPP_

#include <alphagomoku/utils/configs.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace ag
{
	class GameDataStorage;
}

namespace ag
{
	struct GameDataBufferStats
	{
			uint64_t games = 0;
			uint64_t samples = 0;
			uint64_t cross_win = 0;
			uint64_t draws = 0;
			uint64_t circle_win = 0;
			uint64_t game_length = 0;

			std::string toString() const;
			GameDataBufferStats& operator+=(const GameDataBufferStats &other) noexcept;
	};

	class GameDataBuffer
	{
		private:
			std::vector<GameDataStorage> buffer_data;
			GameConfig game_config;
		public:
			GameDataBuffer() noexcept = default;
			GameDataBuffer(GameConfig cfg) noexcept;
			GameDataBuffer(const std::string &path);

			const GameConfig& getConfig() const noexcept;
			void clear() noexcept;
			int numberOfGames() const noexcept;
			int numberOfSamples() const noexcept;
			void append(const GameDataBuffer &other);
			void addGameData(const GameDataStorage &game);
			const GameDataStorage& getGameData(int index) const;
			GameDataStorage& getGameData(int index);
			void removeFromBuffer(int index);
			void removeRange(int from, int to);

			void save(const std::string &path) const;
			void load(const std::string &path);

			GameDataBufferStats getStats() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_GAMEDATABUFFER_HPP_ */
