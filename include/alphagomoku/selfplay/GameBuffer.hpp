/*
 * GameBuffer.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_

#include <mutex>
#include <string>
#include <vector>

namespace ag
{
	class Game;
}

namespace ag
{
	struct GameBufferStats
	{
			uint64_t played_games = 0;
			uint64_t positions = 0;
			uint64_t cross_win = 0;
			uint64_t draws = 0;
			uint64_t circle_win = 0;
			uint64_t game_length = 0;

			std::string toString() const;
			GameBufferStats& operator+=(const GameBufferStats &other) noexcept;
	};

	class GameBuffer
	{
		private:
			mutable std::mutex buffer_mutex;
			std::vector<Game> buffer_data;
			std::vector<int> game_length_stats;
		public:
			GameBuffer() = default;
			GameBuffer(const std::string &path);

			void clear() noexcept;
			int size() const noexcept;
			void add(const GameBuffer &other);
			void addToBuffer(const Game &game);
			const Game& getFromBuffer(int index) const;
			Game& getFromBuffer(int index);
			void removeFromBuffer(int index);
			void removeRange(int from, int to);

			void save(const std::string &path) const;
			void load(const std::string &path);

			GameBufferStats getStats() const noexcept;
			bool isCorrect() const noexcept;

			const std::vector<int>& getGameLengthStats() const noexcept;
			void updateGameLengthStats(const std::vector<int> &indices);

			std::string generatePGN(bool fullGameHistory = false);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_ */
