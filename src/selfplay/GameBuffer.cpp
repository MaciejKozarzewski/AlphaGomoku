/*
 * GameBuffer.cpp
 *
 *  Created on: Mar 7, 2021
 *      Author: maciek
 */

#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <stddef.h>
#include <iostream>
#include <stdexcept>

namespace ag
{
	GameBufferStats& GameBufferStats::operator+=(const GameBufferStats &other) noexcept
	{
		this->played_games += other.played_games;
		this->positions += other.positions;
		this->cross_win += other.cross_win;
		this->draws += other.draws;
		this->circle_win += other.circle_win;
		this->game_length += other.game_length;
		return *this;
	}
	std::string GameBufferStats::toString() const
	{
		std::string result;
		result += "----GameBufferStats----\n";
		result += "played games  = " + std::to_string(played_games) + '\n';
		result += "positions     = " + std::to_string(positions) + '\n';
		result += "cross  = " + std::to_string(cross_win) + '\n';
		result += "draws  = " + std::to_string(draws) + '\n';
		result += "circle = " + std::to_string(circle_win) + '\n';
		result += "avg length = " + std::to_string((float) game_length / played_games) + '\n';
		return result;
	}

	GameBuffer::GameBuffer(const std::string &path)
	{
		load(path);
	}

	void GameBuffer::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.clear();
		stats = GameBufferStats();
	}
	int GameBuffer::size() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.size();
	}
	void GameBuffer::addToBuffer(const Game &game)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.push_back(Game(game));

//		stats.played_games++;
//		stats.positions += game.getNumberOfSamples();
//		stats.game_length += game.length();
//		if (game.getOutcome() == 0)
//			stats.draws++;
//		else
//		{
//			if (game.getOutcome() == 1)
//				stats.cross_win++;
//			else
//				stats.circle_win++;
//		}
	}
	const Game& GameBuffer::getFromBuffer(int index) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	Game& GameBuffer::getFromBuffer(int index)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	void GameBuffer::save(const std::string &path) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		Json json(JsonType::Array);
		SerializedObject so;
		for (size_t i = 0; i < buffer_data.size(); i++)
			json[i] = buffer_data[i].serialize(so);

		FileSaver fs(path);
		fs.save(json, so, -1, true);
		fs.close();
	}
	void GameBuffer::load(const std::string &path)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		FileLoader fl(path, true);
		size_t size = fl.getJson().size();
		for (size_t i = 0; i < size; i++)
			buffer_data.push_back(Game(fl.getJson()[i], fl.getBinaryData()));

//		size_t offset = 0;
//		SerializedObject so(path);
//		int _board_size;
//		so.load(&_board_size, offset, sizeof(int));
//		offset += sizeof(int);
//
//		if (board_size != 0 && _board_size != board_size)
//			throw std::logic_error("board size mismatch");
//		else
//			board_size = _board_size;
//
//		int size;
//		so.load(&size, offset, sizeof(int));
//		offset += sizeof(int);
//		for (int i = 0; i < size; i++)
//			buffer_data.push_back(Game(so, offset));
	}
	GameBufferStats GameBuffer::getStats() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		GameBufferStats stats;
		stats.played_games = buffer_data.size();
		for (size_t i = 0; i < buffer_data.size(); i++)
		{
			stats.positions += buffer_data[i].getNumberOfSamples();
			stats.game_length += buffer_data[i].length();
			if (buffer_data[i].getOutcome() == GameOutcome::DRAW)
				stats.draws++;
			else
			{
				if (buffer_data[i].getOutcome() == GameOutcome::CROSS_WIN)
					stats.cross_win++;
				else
					stats.circle_win++;
			}
		}
		return stats;
	}
	bool GameBuffer::isCorrect() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		for (size_t i = 0; i < buffer_data.size(); i++)
			if (buffer_data[i].isCorrect() == false)
				return false;
		return true;
	}

} /* namespace alfa */

