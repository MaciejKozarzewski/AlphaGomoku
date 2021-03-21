/*
 * GameBuffer.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_

#include <alphagomoku/selfplay/Game.hpp>
#include <mutex>
#include <string>
#include <vector>

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

			void print() const;
			GameBufferStats& operator+=(const GameBufferStats &other);
			std::string toString() const;
	};

	class GameBuffer
	{
		private:
			mutable std::mutex buffer_mutex;
			std::vector<Game> buffer_data;
		public:
			GameBufferStats stats;

			GameBuffer() = default;
			GameBuffer(const std::string &path);

			int getNumberOfSamples();
			void addToBuffer(const Game &game);
			Game& getFromBuffer(int index);
			void save(const std::string &path) const;
			void load(const std::string &path);
			void clear();
			GameBufferStats getStats();
			bool isCorrect() const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_ */
