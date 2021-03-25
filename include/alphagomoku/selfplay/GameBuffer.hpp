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

			std::string toString() const;
			GameBufferStats& operator+=(const GameBufferStats &other) noexcept;
	};

	class GameBuffer
	{
		private:
			mutable std::mutex buffer_mutex;
			std::vector<Game> buffer_data;
		public:

			GameBuffer() = default;
			GameBuffer(const std::string &path);

			void clear() noexcept;
			int size() const noexcept;
			void addToBuffer(const Game &game);
			const Game& getFromBuffer(int index) const;
			Game& getFromBuffer(int index);

			void save(const std::string &path) const;
			void load(const std::string &path);

			GameBufferStats getStats() const noexcept;
			bool isCorrect() const noexcept;

			std::string generatePGN(const std::string &crossPlayer, const std::string &circlePlayer, bool justOutcomes = true);
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAMEBUFFER_HPP_ */
